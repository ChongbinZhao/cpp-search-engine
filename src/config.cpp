#include "config.hpp"

size_t  _size = 0;
list<node*>  _info = list<node*> ();

// ��ȡ�����ļ��е���Ϣ
bool  Read(const string fileName = "my.conf") {
    cout << " loading " << fileName << endl;
    if(fileName.size() == 0) return false;

    ifstream fp;
    fp.open(fileName,std::ios::in);         // ֻ��
    if(fp.is_open() == false) return false; // �ļ���ʧ��
    
    while(fp.eof() == false) {
        string buf("");
        getline(fp,buf);
        //std::cout<<buf<<std::endl;
        if(buf.size() == 0) continue;
        
        // �ų�ע�ͣ�[]���⣬���Ϸ���key(����_����ĸ��ͷ����)
        if(buf[0] != '_' &&  !is_char(buf[0])) {
            continue;   // ������һ�����ݶ�ȡ
        }

        // ��ȡkey value�� = �ָ�
        size_t p = buf.find("=");
        string key = buf.substr(0,p);
        string value = buf.substr(p+1);
        //std::cout<<key<<" "<<value<<std::endl;
        
        // ȥ���ո�
        ltrim(key);
        rtrim(key);
        ltrim(value);
        rtrim(value);
        
        node* info = new node(key,value);
        _info.push_back(info);
        _size++;
    }
    
    cout << fileName << " load sucess" << endl;
    fp.close();
    return true;
}   

// �õ��ַ������͵� value
const string  GetString(const string key) {
    for(auto& info : _info) {
        if(info->_key == key) return info->_value;
    }
    return "";
}

// �õ�int ���͵����ݣ�Ĭ��ֵΪ del
int  GetInt(const string key,const int del) {
    for(auto& info : _info)
        if(info->_key == key) return std::atoi(info->_value.c_str());
    return del;
}

// ����ַ�����߿ո�
void  ltrim(string& str) {
    int p = 0;
    while(p < str.size() && (str[p] == 10 || str[p] == 13 || str[p] == 32)) p++;
    string tmp = str.substr(p);
    str = tmp;
}

// ����ַ����ұ߿ո�           
void  rtrim(string& str) {
    int p = str.size() - 1;
    while(p >= 0 && (str[p] == 10 || str[p] == 13 || str[p] == 32)) 
        str.erase(p--);
}

inline bool  is_char(char ch) {
    if(ch >= 'a' && ch <= 'z') return true;
    if(ch >= 'A' && ch <= 'Z') return true;
    return false;
}
