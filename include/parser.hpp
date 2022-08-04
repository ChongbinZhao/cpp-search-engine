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

//һ���ĵ���Ϣ�ĸ���
struct DocInfo{
    string _title;      //�ĵ�����
    string _url;        //�ĵ��ĵ�ַ
    string _content;    //�ĵ�������
};
//boost �� .html�ļ���·��
static string g_input_path = "./data/input/";

// �����ĵ���·�� ����ǰ׺
static string g_url_head = "https://www.boost.org/doc/libs/1_53_0/doc/";

//�õ�����html�ļ�·��
bool GetFilePath(const string& input_path,vector<string>* file_list);

//�ҵ�����  <title> </title>
bool ParseTitle(const string& html,string* title);

//�ҵ���ַ·��
bool ParseUrl(const string& file_path,string* url);

//�ҵ�����
bool ParseContent(const string& html,string* content);

//���зִ�
bool ParseFile(const string& file_path,DocInfo* doc_info);

//���ִʺ�Ľ��д���ļ�
void WriteOutput(const DocInfo& doc_info,std::ofstream& ofstream);

//���ִʺ�Ľ��д��MySQL���ݿ�
bool WriteToMySQL(MYSQL* con, const DocInfo& doc_info, int count);