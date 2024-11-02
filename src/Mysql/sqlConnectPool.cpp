#include "sqlConnectPool.h"

SqlConnPool *SqlConnPool::getInstance()
{
    static SqlConnPool pool;
    return &pool;
}
std::shared_ptr<MysqlConn> SqlConnPool::getConnection()
{
    std::unique_lock<std::mutex> locker(mutex_);
    if(connectionQueue_.empty()){
        while(connectionQueue_.empty()){
            if(std::cv_status::timeout == cond_.wait_for(locker,std::chrono::milliseconds(timeOut_))){
                if(connectionQueue_.empty()){
                    continue;
                }
            }
        }
    }
    // 有可用的连接
    // 如何还回数据库连接？
    // 使用共享智能指针并规定其删除器
    // 规定销毁后调用删除器，在互斥的情况下更新空闲时间并加入数据库连接池
    // 这个位置不能使用unique_ptr，因为我们禁止发生拷贝，如果是unique的话是会发生拷贝的，而share是对对象的一个引用。
    std::shared_ptr<MysqlConn> connptr(connectionQueue_.front(),
                                       [this](MysqlConn *conn)
                                       {
                                           std::lock_guard<std::mutex> locker(mutex_);
                                           conn->refreshAliveTime();
                                           connectionQueue_.push(conn);
                                       });
    connectionQueue_.pop();
    cond_.notify_all();
    return connptr;
}
void SqlConnPool::Init(const std::string &user, const std::string &pwd,
                       const std::string &dbName, const std::string &localHost,
                       uint16_t port, const int connMinSize,
                       const int connMaxSize, const int timeout, const int maxtime)
{
    localHost_ = localHost;
    user_ = user;
    pwd_ = pwd;
    dbName_ = dbName;
    port_ = port;
    minSize_ = connMinSize;
    maxSize_ = connMaxSize;
    timeOut_ = timeout;
    maxIdleTime_ = maxtime;
    currentSize_=0;//我把这个初始化在这个位置了。

    for (int i = 0; i < minSize_; ++i)
    {
        addConn();
    }
    // 开启新线程执行任务
    std::thread producer(&SqlConnPool::proConn, this);
    std::thread recycler(&SqlConnPool::removeConn, this);
    // 设置线程分离，不阻塞在此处
    producer.detach();
    recycler.detach();
}


SqlConnPool::~SqlConnPool() {
    // 释放队列里管理的MySQL连接资源
    while (!connectionQueue_.empty())
    {
        MysqlConn *conn = connectionQueue_.front();
        connectionQueue_.pop();
        delete conn;
        --currentSize_;
    }
}

//生产连接
void SqlConnPool::proConn() {
    while(true){
        // RAII手法封装的互斥锁，初始化即加锁，析构即解锁
        std::unique_lock<std::mutex>locker(mutex_);
        while(!connectionQueue_.empty()){
            //接下来  "释放"  对mutux_的锁，并同时在这等到cond_通知，
            cond_.wait(locker);
        }
        //如果没有到最大的线程数，就进行创建
        if(currentSize_<maxSize_){
            addConn();

            //唤醒被阻塞的线程
            cond_.notify_all();
        }
    }
}
void SqlConnPool::removeConn() {
    while(true){
        //500毫秒检查一次
        std::this_thread::sleep_for(std::chrono::microseconds(500));
        std::lock_guard<std::mutex>lock(mutex_);//这个锁更加轻量，不支持手动的停止，类似cond_.wait(locker);不支持
        while(connectionQueue_.size()>minSize_){
            MysqlConn *conn =connectionQueue_.front();
            if(conn->getAliveTime()>maxIdleTime_){
                //超过了约定的时间
                connectionQueue_.pop();
                delete conn;
                --currentSize_;
            }else {
                break;
            }
        }
    }
}
void SqlConnPool::addConn() {
    MysqlConn *conn=new MysqlConn;
    conn->connect(user_,pwd_,dbName_,localHost_,port_);
    conn->refreshAliveTime();//刷新这个连接的连接时间。
    connectionQueue_.push(conn);//放入队列当中
    ++currentSize_;//数量增加
}