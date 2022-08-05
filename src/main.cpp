#include <iostream>
#include <string>
#include "httplib.h"
#include "searcher.hpp"
#include "config.hpp"
#include "parser.hpp"
#include "SetTitle.hpp"

// Ĭ�ϼ����˿�
#define LISTENPORT  19998
#define FILENAME    "my.conf"

searcher::Searcher searchf;
bool ret = true;

void GetWebData(const httplib::Request& req,httplib::Response& resp){
    //  ��������������������ͨ������query���͸�������
    if(!req.has_param("query")){
        resp.set_content("Empty query!","text/plain;charset=utf-8");
        return ;
    }
    
    //  ��ȡ����ֵ
    string query = req.get_param_value("query");
    string results;
    searchf.Search(query,&results);
    resp.set_content(results,"application/json;charset=utf-8");
    
    cout<< "The size of result is : "<< results.size()<<endl<<endl;
}


//  ������������þ����ó������ػ����̵ķ�ʽ�ں�̨����
bool init(int argc,char* argv[]) {
    Read(FILENAME);             // ��ȡ�����ļ��е���Ϣ����������������_info�б���

    ProTitle pt(argc,argv);
    ret = pt.MoveOsEnv();       // �����������
    if(ret == false) {
        cout << "move os environ error" << endl;
        return false;
    }
    
    cout << GetString("pro_name") << endl; // �ػ����̵����ƣ�����������
    ret = pt.SetProcTitle(GetString("pro_name").c_str());
    if(ret == false) {
        cout << "set title error" << endl;
        return false;
    }

    if(GetInt("isDaemon",0) == 1) {
        cout << "daemon ing" << endl;
        if(!pt.SetDaemon())
            cout << "daemon error" << endl;
    }

    return true;
}


int main(int argc,char* argv[]){
    // ���ý��̱��⣬���ػ����̣���֤��Զ���������ں�̨����ʽ����
    init(argc, argv); 

    // Init()����redis���ӳغ�����
    ret = searchf.Init();
    if(ret == false){
        cout<<"Searcher init failed!"<<endl;
        return -1;
    }
    
    using namespace httplib;
    Server server;

    // ����ǰ���ļ��ĸ�Ŀ¼: wwwroot = "./WWW"
    ret = server.set_base_dir( GetString("wwwroot").c_str());
    
    // ���� �ص����� �� ��ַ ӳ��
    server.Get("/searcher",GetWebData);
    
    // ��������,�����̻�������listen����
    ret = server.listen( GetString("listenIp").c_str(),GetInt("listenPort",19998));
    if(!ret) cout << " server.listen() failed! " << endl;
    
    return 1;
}
