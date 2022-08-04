#pragma once

#include "cppjieba/Jieba.hpp"
#include "config.hpp"
#include "redis.hpp"
#include "redis_connection_pool.hpp"

#include <stdint.h>
#include <iostream>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <cstdlib> 
#include <mysql/mysql.h>

using std::cout;
using std::endl;
using std::string;
using std::unordered_map;
using std::vector;
using std::atoi;


namespace searcher {
    //  数据库信息
    static string username = "root";
    static string password = "ZCBzcb_2589";
    static string databaseName = "yourdb";

    //  正排索引的存储结构体
    struct frontIdx{
        int64_t _docId;
        string  _title;
        string  _url;
        string  _content;
    };

    //  倒排索引存储的结构体(IDF在建完倒排索引才能计算)
    struct backwardIdx{
        int64_t _docId;
        double  _TF;
        double  _IDF;
        double  _weight;
        string  _word;
    };

    //  建立索引模块
    class Index{
        public:
            Index();
            //  查找正排索引
            const frontIdx* GetFrontIdx(const int64_t doc_id);
            //  查倒排索引
            const vector<backwardIdx>* GetBackwardIdx(const string& key);
            //  建立倒排索引 与 正排索引
            bool Build();
            //  jieba分词 对语句进行分词
            void CutWord(const string& input,vector<string>* output);
        private:
            //  根据一行 预处理 解析的文件，得到一个正排索引的节点
            frontIdx* BuildForward(MYSQL_ROW& row);
            //  根据正排索引节点，构造倒排索引节点
            void BuildInverted(const frontIdx& doc_info);
        private:
            //  正排索引
            vector<frontIdx> forward_index;
            //  倒排索引（哈希表）
            unordered_map<string,vector<backwardIdx> > inverted_index;
            //  jieba分词
            cppjieba::Jieba jieba;
    };

    //  搜索模块
    class Searcher {
        public:
            Searcher(): index(new Index())
            {} 
            //  初始化：创建redis连接池，构建指定文档的索引
            bool Init();
            //  指定文本进行搜索
            bool Search(const string& query,string* output);
        private:
            //  得到关键字前后的数据，在前端页面显示的文本
            string GetShowContent(const string& content,const string& word);
        public:
            //  redis连接池
            redis_pool* m_RedisConnectionPool;
        private:
            //  需要索引进行搜索
            Index* index; 
    };
}
