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
    Redis* GetRedisConnection();                //  ��ȡһ��redisʵ��
    bool ReleaseRedisConnection(Redis* con);    //  �ͷ�����
    void DestroyRedisPool();                    //  ����redis���ӳ�

    static redis_pool* GetInstance();           //  ��������ģʽ��ȡredis���ӳ�

    void Init(string ip, int port, int MaxCon); //  ��ʼ��

private:
    redis_pool();                               //  ���캯��
    ~redis_pool();                              //  ��������

private:
    int m_MaxCon;                               //  �Զ���redis���������
    int m_BusyCon;                              //  ��ռ�õ�redis����
    int m_FreeCon;                              //  ���е�redis����

    locker lock;                                //  ����ͻ���ͨ��������ͬά��һ��redis���ӳ�
    list<Redis*> RedisConnectionList;           //  redis���ӳ���������ʵ��
    sem reserve;                                //  �ź���

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