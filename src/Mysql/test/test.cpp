#include <iostream>
#include <memory>
#include <thread>
#include "MysqlConn.h"
#include "sqlConnectPool.h"
using namespace std;
int main(){
    SqlConnPool* pool=SqlConnPool::getInstance();
    pool->Init("ming","111111","ming_database","localHost",3306,1,4,10000,1000000);
    cout<<pool->getConnection().use_count()<<endl;
    shared_ptr<MysqlConn>po = pool->getConnection();
    string s="CREATE TABLE IF NOT EXISTS my_table (id INT PRIMARY KEY, name VARCHAR(50))";
    string ss="INSERT INTO my_table (id, name) VALUES (1, 'Alice')";
    bool o=po->update(s.c_str());
    bool oo=po->update(ss.c_str());
    cout<<o<<endl;
    cout<<oo<<endl;
}