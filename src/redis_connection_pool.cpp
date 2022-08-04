#include "redis_connection_pool.hpp"
#include "redis.hpp"

#include <string>
#include <list>
#include <iostream>
#include <stdlib.h>

using namespace std;


//  构造函数
redis_pool::redis_pool()
{
    m_BusyCon = 0;
    m_FreeCon = 0;
}


//  单例懒汉模式返回连接池实例
redis_pool* redis_pool::GetInstance()
{
    static redis_pool ConnectionPool;
    return &ConnectionPool;
}


//  redis连接池信息初始化
void redis_pool::Init(string ip, int port, int MaxCon)
{
    m_ip = ip;
    m_port = port;

    for(int i=0; i<MaxCon; i++)
    {
        Redis* con = new Redis();
        con->connect(m_ip, m_port); //  连接失败connect函数会print
        RedisConnectionList.push_back(con);
        m_FreeCon++;
    }

    reserve = sem(m_FreeCon);
    m_MaxCon = m_FreeCon;
}


//  从连接池获取一个空闲的redis连接
Redis* redis_pool::GetRedisConnection()
{
    Redis* con = NULL;

    if(RedisConnectionList.size()==0) return NULL;

    //  取出一个连接，信号量原子减1，当为0时则等待资源
    reserve.wait();

    //  对redis连接池（链表）操作前要先上锁
    lock.lock();

    con = RedisConnectionList.front();
    RedisConnectionList.pop_front();

    m_FreeCon--;
    m_BusyCon++;

    //  解锁
    lock.unlock();
    return con;
}


//  释放当前redis连接，并它重新压入列表中
bool redis_pool::ReleaseRedisConnection(Redis* con)
{
    if(con==NULL) return false;

    lock.lock();

    RedisConnectionList.push_back(con);
    m_FreeCon++;
    m_BusyCon--;

    lock.unlock();

    reserve.post();
    return true;
}


//  销毁redis连接池
void redis_pool::DestroyRedisPool()
{
    lock.lock();
    if(RedisConnectionList.size() > 0)
    {
        for(auto& con : RedisConnectionList)
        {
            con->close();
        }
        m_FreeCon = 0;
        m_BusyCon = 0;
    }
    lock.unlock();
}


//  以析构的方式销毁连接池
redis_pool::~redis_pool()
{
    DestroyRedisPool();
}


//  RAII机制调用redis连接
connectionRAII::connectionRAII(Redis** con, redis_pool* ConnectionPool)
{   
    *con = ConnectionPool->GetRedisConnection();
    conRAII = *con;
    poolRAII = ConnectionPool;
}


//  RAII机制析构
connectionRAII::~connectionRAII()
{
    poolRAII->ReleaseRedisConnection(conRAII);
}