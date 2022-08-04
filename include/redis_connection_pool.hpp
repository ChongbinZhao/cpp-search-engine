#ifndef REDISCONNECTIONPOOL_H
#define REDISCONNECTOINPOLL_H

#include "lock.hpp"
#include "redis.hpp"

#include <iostream>
#include <string>
#include <list>

using namespace std;


class redis_pool
{
public:
    Redis* GetRedisConnection();                //  获取一个redis实例
    bool ReleaseRedisConnection(Redis* con);    //  释放连接
    void DestroyRedisPool();                    //  销毁redis连接池

    static redis_pool* GetInstance();           //  单例懒汉模式获取redis连接池

    void Init(string ip, int port, int MaxCon); //  初始化

private:
    redis_pool();                               //  构造函数
    ~redis_pool();                              //  析构函数

private:
    int m_MaxCon;                               //  自定义redis最大连接数
    int m_BusyCon;                              //  已占用的redis连接
    int m_FreeCon;                              //  空闲的redis连接

    locker lock;                                //  多个客户端通过锁来共同维护一个redis连接池
    list<Redis*> RedisConnectionList;           //  redis连接池用链表来实现
    sem reserve;                                //  信号量

public:
    string m_ip;
    int m_port;
};


class connectionRAII
{
public:
    connectionRAII(Redis** con, redis_pool* ConnectionPool);
    ~connectionRAII();

private:
    Redis* conRAII;
    redis_pool* poolRAII;
};

#endif