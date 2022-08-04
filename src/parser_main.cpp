#include "parser.hpp"
#include <mysql/mysql.h>


#if 0
int main(){
    //建立MySQL连接
    string username = "root";
    string password = "xxxxxxxx";
    string databaseName = "yourdb";

    MYSQL* con = NULL;
    con = mysql_init(con);

    if(con==NULL){
        cout<<"mysql_init Error!"<<endl;
        return -1;
    }
    
    con = mysql_real_connect(con, "localhost", username.c_str(), password.c_str(), databaseName.c_str(), 3306, NULL, 0);

        if(con==NULL){
        cout<<"mysql_real_connect Error!"<<endl;
        return -1;
    }

    //用于存放每一个html文件的路径
    vector<string> file_list;

    //得到所有html文件路径
    bool ret = GetFilePath("./data/input/", &file_list);
    if(ret == false){
        cout<<"GetFilePath() Failed!"<<endl;
        return -1;
    }

    //对每个html文件进行处理
    int count = 1;
    for(const auto& file : file_list){
        DocInfo doc_info;
        int ret = ParseFile(file, &doc_info);
        if(ret==false){
            cout<<file<<" can't be processed!"<<endl;
            count++;
            continue;
        }
        
        //将该条html记录写进数据表里
        if(!WriteToMySQL(con, doc_info, count)){
            cout<<count<<" of "<<file_list.size()<<" WriteToMySQL Failed!"<<endl;
        }

        if(count%100==0 || count==file_list.size()){
            cout<<count<<" of "<<file_list.size()<<" HTML files Processed!"<<endl;
        }

        count++;
    }

    //用完后要关闭数据库连接
    mysql_close(con);
    return 0;
}
#endif
