#pragma once

#include <fstream>
#include <list>
#include <iostream>

using namespace std;

#define  FILENAME "my.conf"


struct node {
    string _key;
    string _value;
    node(const string& key,const string& value)
        :_key(key),_value(value)
        {}
};

extern  list<node*>      _info; //my.conf�ļ���Ĳ���������ֵ�����������б�����
extern  size_t           _size; //_info��Ԫ�ظ���
    
const string GetString(const string key);        // �õ��ַ������͵� value
int GetInt(const string key,const int del);      // �õ�int ���͵����ݣ�Ĭ��ֵΪ del
bool Read(const string fileName);           // ��ȡ�����ļ��е���Ϣ
void ltrim(string& str);                    // ����ַ�����߿ո�
void rtrim(string& str);                    // ����ַ����ұ߿ո�
inline bool is_char(char ch);               // �ж��Ƿ�Ϊ��ĸ
