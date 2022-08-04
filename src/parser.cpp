#include "parser.hpp"
#include <mysql/mysql.h>

using std::cout;
using std::endl;
using std::string;
using std::unordered_map;
using std::vector;


bool GetFilePath(const string& input_path,vector<string>* file_list){
    namespace fs = boost::filesystem;
    fs::path root_path(input_path);
    if(fs::exists(root_path) == false){
        cout<<input_path<<" not exists"<<endl;
        return false;
    }

    fs::recursive_directory_iterator end_iter;
    for(fs::recursive_directory_iterator iter(root_path); 
        iter != end_iter; iter++){
            //当前路径为目录时，直接跳过
            if(fs::is_regular_file(*iter) == false){
                continue;
            }
            
            //当前文件不是 .html 文件，直接跳过
            if(iter->path().extension() != ".html"){
                continue;
            }

            //得到的路径加入到 vector 数组中
            file_list->push_back(iter->path().string());
        }
    
    return true;
}

//找到标题  <title> </title>
bool ParseTitle(const string& html,string* title){
    size_t begin = html.find("<title>");
    if(begin == string::npos){
        cout<<"title not find"<<endl;
        return false;
    }

    size_t end = html.find("</title>",begin);
    if(end == string::npos){
        cout<<"title not find"<<endl;
        return false;
    }

    begin += string("<title>").size();
    if(begin >= end){
        cout<<"title pos info error"<<endl;
        return false;
    }

    *title = html.substr(begin,end - begin);
    return true;
}

// 本地路径形如:
// ../data/input/html/thread.html
// 在线路径形如:
// https://www.boost.org/doc/libs/1_53_0/doc/html/thread.html
bool ParseUrl(const string& file_path,string* url){
    // cout<<"file_path: "<<file_path<<endl;
    // cout<<"*url: "<<*url<<endl;
    // cout<<"g_input_path: "<<g_input_path<<endl;
    string url_tail = file_path.substr(g_input_path.size());//  g_input_size = ./data/input/
    // cout<<"url_tail: "<<url_tail<<endl;
    // cout<<"g_url_head: "<<g_url_head<<endl;
    *url = g_url_head + url_tail;// g_url_head = https://www.boost.org/doc/libs/1_53_0/doc/
    // cout<<"new *url: "<<*url<<endl<<endl;

    return true;
}

bool ParseContent(const string& html,string* content){
    bool is_content = true;
    for(auto c : html){
        if(is_content == true){
            if(c == '<'){
                //之后对<>中的内容进行忽略处理
                is_content = false;
            }
            else{
                if(c == '\n'){
                    c = ' ';
                }
                content->push_back(c);
            }
        }
        else{
            if(c == '>'){
                is_content = true;
            }
            //忽略标签中的内容 <a> 
        }
    }
    return true;
}

bool ParseFile(const string& file_path,DocInfo* doc_info){//file_path是一个html文件的路径
    string html;
    bool ret = common::Util::Read(file_path,&html);//读到的全部内容以字符串的形式存在里面html里面
    if(ret == false){
        cout<<file_path<< " file read error"<<endl;
        return false;
    }

    ret = ParseTitle(html,&doc_info->_title);
    if(ret == false){
        cout<<"title analysis error "<<endl;
        return false;
    }

    ret = ParseUrl(file_path,&doc_info->_url);
    if(ret == false){
        cout<<"Url analysis error "<<endl;
        return false;
    }

    ret = ParseContent(html,&doc_info->_content);
    if(ret == false){
        cout<<"content analysis error "<<endl;
        return false;
    }
    return true;
}


void WriteOutput(const DocInfo& doc_info,std::ofstream& ofstream){
    ofstream<<doc_info._title<<"\3"<<doc_info._url
            <<"\3"<<doc_info._content<<endl;
}


bool WriteToMySQL(MYSQL* con, const DocInfo& doc_info, int count){
    if(con == NULL) cout<<mysql_error(con)<<endl;
    /*数据库记录格式为(doc_id, title, url, content)*/

    //这样插入会存在转义字符的问题，使得一部分网页无法插入MySQL
    // string sql_insert = "INSERT INTO HTML_TABLE VALUES(";
    // sql_insert += "default";//自增长doc_id
    // sql_insert += ", \"";
    // sql_insert += doc_info._title;
    // sql_insert += "\", \"";
    // sql_insert += doc_info._url;
    // sql_insert += "\", \"";
    // sql_insert += doc_info._content;
    // sql_insert += "\")";
    // int ret = mysql_query(con, sql_insert.c_str());
    // if(ret) {
    //     cout<<"no."<<count<<" sql_insert Failed, the reason is: "<<endl;
    //     cout<<mysql_error(con)<<endl<<endl;
    //     return false;
    //     }


    MYSQL_STMT *stmt = mysql_stmt_init(con);

    string sql_insert = "INSERT INTO HTML_TABLE VALUES(default, ?, ?, ?)";
    if(mysql_stmt_prepare(stmt, sql_insert.c_str(),sql_insert.size())){
        cout<<"no."<<count<<" mysql_stmt_prepare failed!"<<endl;
        return false;
    }

    MYSQL_BIND params[3];
    memset(params, 0, sizeof(params));
    params[0].buffer_type = MYSQL_TYPE_STRING;
    params[0].buffer = (char*)doc_info._title.data();
    params[0].buffer_length = doc_info._title.size();

    params[1].buffer_type = MYSQL_TYPE_STRING;
    params[1].buffer = (char*)doc_info._url.data();
    params[1].buffer_length = doc_info._url.size();

    params[2].buffer_type = MYSQL_TYPE_STRING;
    params[2].buffer = (char*)doc_info._content.data();
    params[2].buffer_length = doc_info._content.size();

    mysql_stmt_bind_param(stmt, params);
    int ret = mysql_stmt_execute(stmt);

    //表示执行失败
    if(ret) {
        cout<<"no."<<count<<" mysql_stmt_execute Failed, the reason is: "<<endl;
        cout<<mysql_error(con)<<endl<<endl;
        return false;
        }
        
    return true;
}
