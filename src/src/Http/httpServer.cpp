
#include "httpServer.h"
#include "httpRequest.h"
#include "sqlConnectPool.h"

HttpServer::HttpServer(EventLoop *loop, const InetAddress& listenAddr,const std::string& name,int loopThreadNum,
            const std::string sqlUser, const std::string sqlPwd, const std::string dbName, const std::string localHost, 
            uint16_t sqlPort, int sqlPoolMinNum,int sqlPoolMaxNum,int sqlTimeOut,int sqlMaxLiveTime,
            TcpServer::Option option
            )
  : server_(loop, listenAddr, name, option)
{
    LOG_DEBUG<<"这个是把httpServer 中的 setConnectionCallback";
    server_.setConnectionCallback(
        std::bind(&HttpServer::onConnection, this, std::placeholders::_1));

    //这个是把httpServer中的HttpServer::onMessage与TcpServer的绑定
    server_.setMessageCallback(
        std::bind(&HttpServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    
    //这个是设置写完的回调，可以关闭连接。
    server_.setWriteCompleteCallback(
        std::bind(&HttpServer::onDestroy,this,std::placeholders::_1));
    //初始化数据库 ,后边的参数是按照默认的
    LOG_DEBUG<<"ready to connect the sql";
    SqlConnPool::getInstance()->Init(sqlUser,sqlPwd,dbName,localHost,sqlPort,sqlPoolMinNum,sqlPoolMaxNum,sqlTimeOut,sqlMaxLiveTime);

    server_.setThreadNum(loopThreadNum);
    srcDir_ = getcwd(nullptr, 256);
    strcat(srcDir_, "/resources/");
}

//这个是在tcpserver中当新连接建立得时候会被调用。
/**
 * ioLoop->runInLoop(
 *      std::bind(&TcpConnection::connectEstablished, conn));
 * 
 * 这个初始化就是创建一个和user_
 */
void HttpServer::onConnection(const TcpConnectionPtr& conn)
{
    // //这个里边的fd需要再仔细思考一下，是不是正确的。
    // const sockaddr_in* addr=conn->peerAddress().getSockAddr();
    // int fd = conn->socket_->fd();
    // users_[fd].init(fd,*addr);
    if (conn->connected())
    {
        LOG_DEBUG << "new Connection arrived";
    }
    else 
    {
        LOG_INFO << "Connection closed";
    }
}

//消息发送完的业务处理。
void HttpServer::onDestroy(const TcpConnectionPtr& conn){
    conn->shutdown(); //这个发送消息后直接断开。
    
}


// 有消息到来时的业务处理
void HttpServer::onMessage(const TcpConnectionPtr& conn,
                           Buffer* buf,
                           Timestamp receiveTime)
{
    HttpResponse response_;
    LOG_DEBUG<< "onMessage on : "<<receiveTime.toFormattedString();
    HttpRequest res;
    
    if(!res.parse(*buf))
    {
            //如果解析错误，则直接输出
        LOG_ERROR << "parseRequest failed!";
        conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
        conn->shutdown();
    }
    //判断是不是keep-alieve
    bool close = res.IsKeepAlive();
    LOG_DEBUG<<res.path();
    response_.Init(srcDir_,res.path(),close,200);
    Buffer buff;
    response_.MakeResponse(buff);
    // 响应头
    LOG_DEBUG<<conn->socket_->fd()<<" file: "<<res.path()<<"\tbuff size:"<<buff.readableBytes();
    // 文件
    if(response_.FileLen() > 0  && response_.File()) {
        buff.append(response_.File(),response_.FileLen());
    }
   
    conn->send(&buff);
    
}

void HttpServer::start()
{
    LOG_INFO << "HttpServer[" << server_.name().c_str() << "] starts listening on " << server_.ipPort().c_str();
    server_.start();
}
