#ifndef _REDIS_H_
#define _REDIS_H_

#include <iostream>
#include <string>
#include <string.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <set>
#include <hiredis/hiredis.h>


class Redis
{
public:
	Redis()
	{}
	~Redis()
	{
		this->connect_ = NULL;
		this->reply_ = NULL;
	}
	
    //  连接redis
	bool connect(std::string host, int port)
	{
		this->connect_ = redisConnect(host.c_str(), port);
		if(this->connect_ != NULL && this->connect_->err)
		{
			printf("connect error: %s\n", this->connect_->errstr);
			return 0;
		}
		return 1;
	}
	
    //  关闭redis
    void close(){
        if(this->connect_ == NULL){
            printf("Redis connection is already released!\n");
        }
		else
		{
			redisFree(this->connect_);
		}
        
    }

    //  select
	void select(unsigned short n)
	{
		redisCommand(this->connect_, "SELECT %u", n);//%u 或者 %d 表示整数
	}

    //  get
	std::string get(std::string key)
	{
		this->reply_ = (redisReply * )redisCommand(this->connect_, "GET %s", key.c_str());
		if(reply_->str == 0)
			return std::string();
		std::string str = this->reply_->str;
		/*reply object has to be cleared*/
		freeReplyObject(this->reply_);              
		return str;
	}

    //  hget
	std::string hget(uint64_t key, std::string field)
	{
		this->reply_ = (redisReply * )redisCommand(this->connect_, "HGET %lld %s", key, field.c_str());
		if(reply_->str == 0)
			return std::string();
		std::string str = this->reply_->str;
		freeReplyObject(this->reply_);
		return str;
	}

    //  set
	void set(std::string key, std::string value)
	{
		redisCommand(this->connect_, "SET %s %s", key.c_str(), value.c_str());
	}

	//  expire设置过期时间，以秒为单位
	void expire(std::string key, std::string seconds)
	{
		redisCommand(this->connect_, "EXPIRE %s %s", key.c_str(), seconds.c_str());
	}

#if 0
	//  append
	void append(size_t value)
	{
		redisCommand(this->connect_, "APPEND %d", value);
	}

    //  rpush：从右边开始向链表加入元素
	void rpush(std::string key, size_t value)
	{
		redisCommand(this->connect_, "RPUSH %s %d", key.c_str(), value);
	}

    //  lrange：获取链表区间在[start, stop]的元素
   std::vector<std::string> lrange(std::string key, int start, int stop)
   {
	   this->reply_ = (redisReply * )redisCommand(this->connect_, "LRANGE %s %d %d", key.c_str(), start, stop);
	   std::vector<std::string> vec;
	   for(size_t idx  = 0; idx < this->reply_->elements; ++idx)
	   {
		    	   
			redisReply * childReply = this->reply_->element[idx];
			vec.push_back(childReply->str);
	   }
	 //  std::cout << "size:" << reply_->elements << std::endl;
	   freeReplyObject(this->reply_);
	   return vec;
   }

    //  smembers：获取某个set的全部成员
   std::set<std::pair<uint64_t, double> > smembers(std::string key)
   {
		this->reply_ = (redisReply * )redisCommand(this->connect_, "SMEMBERS %s", key.c_str());
	//	std::cout << "reply_" << reply_ <<  std::endl;
		std::set<std::pair<uint64_t, double> > set;
		
		for(size_t idx = 0; idx < this->reply_->elements; ++idx)
		{
			uint64_t id; 
			double weight;
			redisReply * childReply = this->reply_->element[idx];
			std::string str = childReply->str;
			std::istringstream is(str);
			is >> id >> weight;
			set.insert(std::make_pair(id, weight));
		}
		freeReplyObject(this->reply_);
		return set;
	}

   bool isExists(std::string key)
   {
	   this->reply_ = (redisReply * )redisCommand(this->connect_, "EXISTS %s", key.c_str());
	   bool bo;
	   bo = bool(this->reply_->str);
	   freeReplyObject(this->reply_);
	   return bo; 
   }
#endif

private:
	redisContext * connect_;
	redisReply * reply_;
};


#endif

