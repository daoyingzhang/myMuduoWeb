#pragma once
#include <string>
#include <mysql/mysql.h>
#include <chrono>



class MysqlConn{

public:
    //初始化
    MysqlConn();
    ~ MysqlConn();
    //连接数据库
    bool connect(const std::string& user,const std::string& pwd,
                 const std::string& dbName,const std::string& localHost="127.0.0.1",
                 const uint16_t port=8080 );
    
    //更新数据库
    bool update(const char* sql);
    //查询
    bool query(const char* sql);
    //遍历查询到的结果集
    bool next();
    //得到结果集的字段值
    std::string value(int index);
    //事务操作
    bool transaction();
    //提交事务
    bool ccommit();
    //事务回滚
    bool rollback();
    //刷新起始的空闲时间点
    void refreshAliveTime();
    //计算连接的存活时长。
    long long getAliveTime(); 

    //拿到结果集，因为外边查询之类的函数时封装好的，所以必须留一个对外的接口。
    MYSQL_RES* getResult(){return result_;}
private:
    void freeResult();
    //数据库连接成功后数据库的接口会保存在conn_中
    MYSQL * conn_=nullptr;
    // 查询或者什么后数据是要保存在结果集中的
    MYSQL_RES* result_ = nullptr;
    MYSQL_ROW row_ = nullptr;

    // 绝对始终
    std::chrono::steady_clock::time_point m_alivetime;
};