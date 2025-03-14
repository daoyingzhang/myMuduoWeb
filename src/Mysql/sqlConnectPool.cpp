#include "sqlConnectPool.h"


SqlConnPool *SqlConnPool::getInstance()
{
    static SqlConnPool pool;
    return &pool;
}
std::shared_ptr<MysqlConn> SqlConnPool::getConnection()
{

    //这个连接有可能长时间不用被数据库给主动断开连接了，所以这个位置要判断一下，如果超时了，就清除队列中的所有连接，然后重新连接
    // 这个位置用代码快的原因是因为如果去掉的话，我们加了locksql锁，但是没进if，会导致下边再死锁。
    {
        LOG_INFO<<"数据库 getConnection";
        std::lock_guard<std::mutex> locksql(mutex_);
        if (connectionQueue_.empty()) {  // **防止 front() 访问空队列**
            LOG_WARN << "数据库连接池为空，创建新连接";//但是这个创建我们让他在下边执行就可以了
        }else {
            MysqlConn *conn = connectionQueue_.front();
            if(!isValid(conn->getSqlConn())){
                LOG_INFO<<"连接超时，需要清除，然后重新建立连接。";
                while(!connectionQueue_.empty()){
                    conn = connectionQueue_.front();
                    delete conn;
                    connectionQueue_.pop();
                    --currentSize_;
                }
            }
        }
    }

    
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
    LOG_INFO<<"begin to delete sqlpool";
    // 释放队列里管理的MySQL连接资源
    while (!connectionQueue_.empty())
    {
        MysqlConn *conn = connectionQueue_.front();
        connectionQueue_.pop();
        delete conn;
        --currentSize_;
    }
}

/*
这个里边有一个说法，最开始数据库的连接池只有一个连接，
只有有线程将这个连接取走后才会创建一个新的连接，再创建一个新的连接，达到最大值后就不再创建了，此时还有要使用数据库的只能等其他用户把线程释放。 
也就是说，连接池保证了在达到最大线程数量之前，一直只有一个
如果 connectionQueue_ 永远不为空（即连接一直未被取走），线程就会一直停留在 wait(locker);，不会创建新连接。此时还可以还回来

*/

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
                cond_.notify_all();
                LOG_INFO<<"delete the connect of the sql whoes time exceed thd limit.";
            }else {
                break;
            }
        }
    }
}
void SqlConnPool::addConn() {
    MysqlConn *conn=new MysqlConn;
    bool creat_Bool=conn->connect(user_,pwd_,dbName_,localHost_,port_);
    LOG_INFO << "add mysql connection:" << creat_Bool;
    conn->refreshAliveTime();//刷新这个连接的连接时间。
    connectionQueue_.push(conn);//放入队列当中
    ++currentSize_;//数量增加
}


bool SqlConnPool::isValid(MYSQL*conn_){
    if (mysql_ping(conn_)) {  // mysql_ping 是 MySQL 提供的 API 来检查连接
        return false;  // 如果返回非零，说明连接失效
    }
    return true;  // 如果连接没有失效
}