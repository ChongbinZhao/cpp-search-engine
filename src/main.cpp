#include <iostream>
#include <string>
#include "httplib.h"
#include "searcher.hpp"
#include "config.hpp"
#include "parser.hpp"
#include "SetTitle.hpp"

// 默认监听端口
#define LISTENPORT  19998
#define FILENAME    "my.conf"

searcher::Searcher searchf;
bool ret = true;

void GetWebData(const httplib::Request& req,httplib::Response& resp){
    //  浏览器将搜索框里的内容通过参数query发送给服务器若
    if(!req.has_param("query")){
        resp.set_content("请求参数错误","text/plain;charset=utf-8");
        return ;
    }
    
    //  获取参数值
    string query = req.get_param_value("query");
    string results;
    searchf.Search(query,&results);
    resp.set_content(results,"application/json;charset=utf-8");
    
    cout<< "The size of result is : "<< results.size()<<endl<<endl;
}


//  这个函数的作用就是让程序以守护进程的方式在后台运行
bool init(int argc,char* argv[]) {
    Read(FILENAME);             // 读取配置文件中的信息，并将参数保存在_info列表里

    ProTitle pt(argc,argv);
    ret = pt.MoveOsEnv();       // 环境变量搬家
    if(ret == false) {
        cout << "move os environ error" << endl;
        return false;
    }
    
    cout << GetString("pro_name") << endl; // 守护进程的名称（方便搜索）
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
    // 设置进程标题，以守护进程（保证永远都是运行在后台）方式运行
    init(argc, argv); 

    // Init()创建redis连接池和索引
    ret = searchf.Init();
    if(ret == false){
        cout<<"Searcher init failed!"<<endl;
        return -1;
    }
    
    using namespace httplib;
    Server server;

    // 设置前端文件的根目录: wwwroot = "./WWW"
    ret = server.set_base_dir( GetString("wwwroot").c_str());
    
    // 建立 回调函数 与 网址 映射
    server.Get("/searcher",GetWebData);
    
    // 建立监听,主进程会阻塞在listen这里
    ret = server.listen( GetString("listenIp").c_str(),GetInt("listenPort",19998));
    if(!ret) cout << " server.listen() failed! " << endl;
    
    return 1;
}
