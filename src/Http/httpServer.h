#pragma once
#include "noncopyable.h"
#include "TcpServer.h"
// #include "httpconn.h"
#include "httpResponse.h"
class HttpRequest;


class HttpServer :noncopyable
{
public:
    using HttpCallback = std::function<void (const HttpRequest&, HttpResponse*)>;

    HttpServer(EventLoop *loop, const InetAddress& listenAddr,const std::string& name,int loopThreadNum,
            const std::string sqlUser, const std::string sqlPwd, const std::string dbName, const std::string localHost, 
            uint16_t sqlPort=3306, int sqlPoolMinNum=1,int sqlPoolMaxNum=4,int sqlTimeOut=1000,int sqlMaxLiveTime=6000000
            ,TcpServer::Option option = TcpServer::kNoReusePort);
    
    void setHttpCallback(const HttpCallback& cb)
    {
        httpCallback_ = cb;
    }
    EventLoop* getLoop() const { return server_.getLoop(); }
    void start();
private:
    void onConnection(const TcpConnectionPtr& conn);
    void onMessage(const TcpConnectionPtr &conn,
                    Buffer *buf,
                    Timestamp receiveTime);
    void onRequest(const TcpConnectionPtr& conn, const HttpRequest& req);
    TcpServer server_;
    HttpCallback httpCallback_;
    //std::unordered_map<int, HttpConn> users_;//这个是用来保存新连接，其实和ConnectionMap connections_;这个差不多一样
    char* srcDir_;
    struct iovec iov_[2];
    int iovCnt_;
    
};