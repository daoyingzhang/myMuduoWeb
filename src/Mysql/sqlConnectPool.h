#pragma once

#include "MysqlConn.h"
#include <thread>//这个是.cpp中用的
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>

class SqlConnPool
{
public:
    SqlConnPool(){};
    static SqlConnPool *getInstance();
    std::shared_ptr<MysqlConn> getConnection();
    void Init(const std::string &user, const std::string &pwd,
              const std::string &dbName, const std::string &localHost,
              uint16_t port, const int connMinSize,
              const int connMaxSize,const int timeout,const int maxldtime);
    ~SqlConnPool();

private:

    // 禁止拷贝构造，移动构造，=的重载
    SqlConnPool(const SqlConnPool &obj) = delete;
    SqlConnPool(const SqlConnPool &&obj) = delete;
    SqlConnPool &operator=(const SqlConnPool &obj) = delete;

    void proConn();
    void removeConn();
    void addConn();

    std::string localHost_;
    std::string user_;
    std::string pwd_;
    std::string dbName_;
    uint16_t port_;
    int minSize_;
    int maxSize_;
    int timeOut_; // 按照毫秒计时的
    int maxIdleTime_ ;

    std::queue<MysqlConn *> connectionQueue_; // 连接池队列
    std::mutex mutex_;
    std::condition_variable cond_;
    int currentSize_;
};


// /* 资源在对象构造初始化 资源在对象析构时释放*/
// class SqlConnRAII {
// public:
//     SqlConnRAII(std::shared_ptr<MysqlConn*> sql, SqlConnPool *connpool) {
//         assert(connpool);
//         sql = connpool->getConnection();
//         sql_ = sql;
//         connpool_ = connpool;
//     }
    
//     ~SqlConnRAII() {
//         if(sql_) { connpool_->FreeConn(sql_); }
//     }
    
// private:
//     std::shared_ptr<MysqlConn*> sql_;
//     SqlConnPool* connpool_;
// };