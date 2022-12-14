#pragma once

#include "util.hpp"
#include "config.hpp"

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <unordered_map>

#include <mysql/mysql.h>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>


using std::cout;
using std::endl;
using std::string;
using std::unordered_map;
using std::vector;

//一个文档信息的概括
struct DocInfo{
    string _title;      //文档标题
    string _url;        //文档的地址
    string _content;    //文档的正文
};
//boost 中 .html文件的路径
static string g_input_path = "./data/input/";

// 在线文档的路径 公共前缀
static string g_url_head = "https://www.boost.org/doc/libs/1_53_0/doc/";

//得到所有html文件路径
bool GetFilePath(const string& input_path,vector<string>* file_list);

//找到标题  <title> </title>
bool ParseTitle(const string& html,string* title);

//找到网址路径
bool ParseUrl(const string& file_path,string* url);

//找到正文
bool ParseContent(const string& html,string* content);

//进行分词
bool ParseFile(const string& file_path,DocInfo* doc_info);

//将分词后的结果写入文件
void WriteOutput(const DocInfo& doc_info,std::ofstream& ofstream);

//将分词后的结果写入MySQL数据库
bool WriteToMySQL(MYSQL* con, const DocInfo& doc_info, int count);