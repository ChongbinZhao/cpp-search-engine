#include "redis_connection_pool.hpp"
#include "redis.hpp"

#include <string>
#include <list>
#include <iostream>
#include <stdlib.h>

using namespace std;


//  ���캯��
redis_pool::redis_pool()
{
    m_BusyCon = 0;
    m_FreeCon = 0;
}


//  ��������ģʽ�������ӳ�ʵ��
redis_pool* redis_pool::GetInstance()
{
    static redis_pool ConnectionPool;
    return &ConnectionPool;
}


//  redis���ӳ���Ϣ��ʼ��
void redis_pool::Init(string ip, int port, int MaxCon)
{
    m_ip = ip;
    m_port = port;

    for(int i=0; i<MaxCon; i++)
    {
        Redis* con = new Redis();
        con->connect(m_ip, m_port); //  ����ʧ��connect������print
        RedisConnectionList.push_back(con);
        m_FreeCon++;
    }

    reserve = sem(m_FreeCon);
    m_MaxCon = m_FreeCon;
}


//  �����ӳػ�ȡһ�����е�redis����
Redis* redis_pool::GetRedisConnection()
{
    Redis* con = NULL;

    if(RedisConnectionList.size()==0) return NULL;

    //  ȡ��һ�����ӣ��ź���ԭ�Ӽ�1����Ϊ0ʱ��ȴ���Դ
    reserve.wait();

    //  ��redis���ӳأ���������ǰҪ������
    lock.lock();

    con = RedisConnectionList.front();
    RedisConnectionList.pop_front();

    m_FreeCon--;
    m_BusyCon++;

    //  ����
    lock.unlock();
    return con;
}


//  �ͷŵ�ǰredis���ӣ���������ѹ���б���
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


//  ����redis���ӳ�
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


//  �������ķ�ʽ�������ӳ�
redis_pool::~redis_pool()
{
    DestroyRedisPool();
}


//  RAII���Ƶ���redis����
connectionRAII::connectionRAII(Redis** con, redis_pool* ConnectionPool)
{   
    *con = ConnectionPool->GetRedisConnection();
    conRAII = *con;
    poolRAII = ConnectionPool;
}


//  RAII��������
connectionRAII::~connectionRAII()
{
    poolRAII->ReleaseRedisConnection(conRAII);
}