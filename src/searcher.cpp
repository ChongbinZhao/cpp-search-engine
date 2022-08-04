#include "searcher.hpp"
#include "util.hpp"

#include <mysql/mysql.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>
#include <jsoncpp/json/json.h>
#include <boost/algorithm/string/case_conv.hpp>
#include <fstream>
#include <string>
#include <algorithm>
#include <cmath>


namespace searcher {
    
    //jieba分词词典的路径
    const char* const DICT_PATH = "./jieba_dict/jieba.dict.utf8";
    const char* const HMM_PATH = "./jieba_dict/hmm_model.utf8";
    const char* const USER_DICT_PATH = "./jieba_dict/user.dict.utf8";
    const char* const IDF_PATH = "./jieba_dict/idf.utf8";
    const char* const STOP_WORD_PATH = "./jieba_dict/stop_words.utf8";

    //  搜索结果内容显示长度
    const int lengthText = 160;

    //  初始化
    Index::Index()
        :jieba(DICT_PATH, HMM_PATH, USER_DICT_PATH, IDF_PATH, STOP_WORD_PATH)
    {
        forward_index.clear();
        inverted_index.clear();
    }


    //查找正排索引
    const frontIdx* Index::GetFrontIdx(const int64_t doc_id){
        if(doc_id < 0 || doc_id >= forward_index.size()){
            return nullptr;
        }
        return &forward_index[doc_id];
    }


    //  查倒排索引
    const vector<backwardIdx>* Index::GetBackwardIdx(const string& key){
        auto it = inverted_index.find(key);
        if(it == inverted_index.end()){
            return nullptr;
        }

        return &(it->second);
    }


    // 建立索引
    bool Index::Build(){
        //  初始化一些变量，用于输出Build()的进度
        cout<<"Begin to build index..."<<endl;
        string line;
        int idx = 0;
        static string progess("@#$%*");

        //  从MySQL的HTML_TABLE读取事先存储好的html网页数据
        MYSQL* con = mysql_init(NULL);

        //  MySQL连接初始化报误
        if(con==NULL){
            cout<<"mysql_init Error!"<<endl;
            return false;
        }
        
        //  连接MySQL，要自己指定用户名、密码和数据库名称
        con = mysql_real_connect(con, "localhost", username.c_str(), password.c_str(), databaseName.c_str(), 3306, NULL, 0);

        //  连接MySQL失败
        if(con==NULL){
            cout<<"mysql_real_connect Error!"<<endl;
            return false;
        }

        //  执行查询语句，将存储在HTML_TABLE的html网页数据提取出来
        if (mysql_query(con, "SELECT * FROM HTML_TABLE")){
            cout<<"SELECT error: "<<mysql_error(con)<<endl;
        }

        //  获取上一次查询的结果
        MYSQL_RES *result = mysql_store_result(con);

        //  数据表的属性个数：理论上为id title url content共4个
        if(mysql_num_fields(result) != 4){
            cout<<"mysql_store_result() failed: "<<mysql_error(con)<<endl;
            return false;
        }

        //  获取结果行数（等同于html页面数量）
        int html_nums = mysql_num_rows(result);

        // 遍历每一行记录获得正排索引，再根据正排索引获得倒排索引
        while(MYSQL_ROW row = mysql_fetch_row(result)){
            //  根据一条MySQL记录获得一个正排索引
            frontIdx* doc_info = BuildForward(row);

            //  根据一个正排索引获得对应的倒排索引
            BuildInverted(*doc_info);

            //  打印部分构建结果，方便了解Build()的进度
            if(doc_info->_docId % 100 == 0){
                cout<<"\r"<<progess[idx % 4]<<"no."<< doc_info->_docId << " done! " <<std::flush;
                idx++;
            }
        }

        //  这一部分开始计算backwardIdX里的TF-IDF权重(标题TF-IDF和正文TF-IDF的线性加权)
        for(auto& word_vector : inverted_index)
        {
            for(auto& backIdx : word_vector.second)
            {
                backIdx._IDF = log(html_nums / word_vector.second.size());
                backIdx._weight = backIdx._TF * backIdx._IDF;
            }
        }

        cout << endl << "Index was built sucessfully! "<<endl;
        return true;
    }


    // jieba分词 对语句进行分词
    void Index::CutWord(const string& input,vector<string>* output){
        jieba.CutForSearch(input,*output);
    }


    //  根据一行记录，得到一个正排索引的节点,并插入到正排数组中
    frontIdx* Index::BuildForward(MYSQL_ROW& row){
        frontIdx doc_info;
        doc_info._docId     = atoi(row[0])-1;
        doc_info._title     = row[1];
        doc_info._url       = row[2];
        doc_info._content   = row[3];

        forward_index.push_back(std::move(doc_info)); 
        return &forward_index.back();
    }


    //  根据正排索引节点，构造倒排索引节点（一次处理一个文档）
    void Index::BuildInverted(const frontIdx& doc_info){
        //  统计关键字作为 标题 和正文的出现次数
        struct WordCnt {
            int _titleCnt;
            int _contentCnt;
            WordCnt()
                :_titleCnt(0),_contentCnt(0)
                {} 
        };
            
        //  存储每一个词及其对应的出现次数(针对一个网页)
        unordered_map<string,WordCnt> wordMap;

        //  针对标题进行分词
        vector<string> titleWord;
        CutWord(doc_info._title,&titleWord);
        for(string word : titleWord){
            // 排除单独符号的影响
            if(word.size() == 0 || (!(word[0] >=  'a' && word[0] <= 'z')\
                    &&  !(word[0] <= 'Z' && word[0] >= 'A') ) )
                continue;
            
            //  全部转为小写
            boost::to_lower(word);
            //  关键词在 标题 中出现时权重加10
            wordMap[word]._titleCnt += 10;
        }

        //  针对正文进行分词
        vector<string> contentWord;
        CutWord(doc_info._content,&contentWord);
        for(string word : contentWord){
            if(word.size() == 0 || !(word[0] >=  'a' && word[0] <= 'z')\
                    &&  !(word[0] <= 'Z' && word[0] >= 'A'))
                continue;
            boost::to_lower(word);
            //  关键词在 正文 中出现时权重加1
            wordMap[word]._contentCnt++;
        }

        //  统计结果，插入到倒排索引中；word_pair为<string, WordCnt>
        for(const auto& word_pair : wordMap){
            backwardIdx backIdx;
            backIdx._docId  = doc_info._docId;
            backIdx._word   = word_pair.first;
            backIdx._TF = (double)(word_pair.second._titleCnt + word_pair.second._contentCnt) / (titleWord.size() + contentWord.size());

            //  unordered_map<string,vector<backwardIdx> > inverted_index;
            //  inverted_index[word_pair.first]记录的是某一个单词在所有网页的权重
            vector<backwardIdx>& back_vector = inverted_index[word_pair.first];
            back_vector.push_back(std::move(backIdx));
        }
    }


    //  初始化：获取redis线程池实例，构建指定文档的索引
    bool Searcher::Init(){
        m_RedisConnectionPool = redis_pool::GetInstance();
        m_RedisConnectionPool->Init("localhost", 6379, 10);
        return index->Build();
    }
    

    //  指定文本进行搜索
    bool Searcher::Search(const string& query,string* output){
        //  分词
        vector<string> words;
        index->CutWord(query,&words);
        cout<<"Split words: "<<words<<endl;
        //  转小写，方便后续处理
        for(auto& word : words)
        {
            boost::to_lower(word);
        }

        //  将单词进行排序：首字符小的在前 > 字符少的在前 > 单词中第一个不同字符小的在前
        std::sort(words.begin(),words.end(),[](const string& l, const string& r)
        {
            if(l[0] < r[0]) return true;
            else if(l[0] > r[0]) return false;
            //  l[0] = r[0]
            else
            {
                if(l.size() < r.size()) return true;
                else if(l.size() > r.size()) return false;
                //  l.size() = r.size()
                else
                {
                    for(int pos=1; pos<l.size(); pos++) //  已知第0个首字符相同，所以从第1个字符开始比较
                    {
                        if(l[pos] < r[pos]) return true;
                        else if(l[pos] > r[pos]) return false;
                        //  l[pos] = r[pos]
                        else continue;
                    }
                }
            }
            return false;
        });

        //  将单词合成一个字符串，当作redis中的key
        string words_sequnce;
        for(auto& word : words) words_sequnce += word;

        //  获取一个redis连接
        Redis* con = NULL;
        connectionRAII instance(&con, m_RedisConnectionPool);   //instance离开作用域析构的同时还会释放con连接，RAII机制就体现在这里

        //  redis查询命中，命中后重置一下过期时间（30分钟=1800秒）
        string res = con->get(words_sequnce);
        if(res.size() != 0 && res != "nil")
        {
            cout<<words_sequnce<<" Hit!"<<endl;
            con->expire(words_sequnce, "1800");
            *output = res;
            return true;
        }
        else if(res=="nil") //搜索的关键词不在倒排索引中的，我们也在redis里把它设置为nil
        {
            cout<<words_sequnce<<" Hit!"<<endl;
            con->expire(words_sequnce, "1800");
            *output = "";
            return true;
        }

        //  redis查询命中未命中，根据分词的结果，进行倒排索引，得到相关文档
        vector<backwardIdx> wordsResult;
        for(string word : words){
            if(word.size() == 0 || !(word[0] >=  'a' && word[0] <= 'z')) 
                continue;
            //const vector<backwardIdx>*
            auto* backList = index->GetBackwardIdx(word);
            if(backList == nullptr){
                //  没有这个关键词
                continue;
            }
            //  插入多个数据
            wordsResult.insert(wordsResult.end(),backList->begin(),backList->end());
        }
        
        //  根据权重排序
        std::sort(wordsResult.begin(),wordsResult.end(),
                [](const backwardIdx& l,const backwardIdx& r){
                    return l._weight > r._weight;
                });

        //  包装
        Json::Value value;
        int cnt = 0;
        vector<bool> st(7000,false); //网页一共只有不到5900个，小于7000
        for(const auto& backidx : wordsResult){
            //  发送给浏览器的网页不超过100个
            if(cnt > 100) break;
            //  根据 id 查找正排索引
            const frontIdx* doc_info = index->GetFrontIdx(backidx._docId);
            //权值最高的单词可能对应的是同一个网页，每一个网页只要展示一次就够了
            if(st[doc_info->_docId]) continue;
            cnt++;
            st[doc_info->_docId] = true;
            
            Json::Value tmp;
            tmp["title"]    = doc_info->_title;
            tmp["url"]      = doc_info->_url;
            tmp["desc"]     = GetShowContent(doc_info->_content,backidx._word);
            value.append(tmp); 
        }
        Json::FastWriter writer;
        *output = writer.write(value);
        if((*output).size()==0)
        {   
            cout<<"Insert nothing to redis, 404 Not Found!"<<endl;
            con->set(words_sequnce, "nil");
            con->expire(words_sequnce, "1800");
        }
        else
        {
            cout<<"Insert \""<<words_sequnce<<"\" into redis! "<<endl;
            con->set(words_sequnce, *output);
            con->expire(words_sequnce, "1800");
        }
        
        return true;
    }


    //  在前端界面只展示关键字前后一部分数据，展示长度由lengthText决定
    string Searcher::GetShowContent(const string& content,const string& word){
        size_t idx = content.find(word);
        string ans("");
        int pos = 0;    //  显示文本开始的位置
        int len = lengthText;   //  截取显示文本的长度
        
        if(idx != string::npos){
            pos = std::max(0,(int)((int)idx-lengthText/2));
        }
        //  关键字不存在,说明关键字只存在于标题中
        ans = content.substr(pos,len);
        ans += "...";
        return ans;
    }
}
