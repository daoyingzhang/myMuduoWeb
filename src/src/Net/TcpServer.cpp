#include <functional>
#include <string.h>

#include "TcpServer.h"
#include "TcpConnection.h"
#include "Logging.h"

// 检查传入的 baseLoop 指针是否有意义
static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
         LOG_ERROR << "mainLoop is null!";
    }
    return loop;
}

TcpServer::TcpServer(EventLoop *loop,
                     const InetAddress &listenAddr,
                     const std::string &nameArg,
                     Option option)
    : loop_(CheckLoopNotNull(loop)),
    ipPort_(listenAddr.toIpPort()),
    name_(nameArg),
    acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),
    threadPool_(new EventLoopThreadPool(loop, name_)),
    connectionCallback_(),
    messageCallback_(),
    writeCompleteCallback_(),
    threadInitCallback_(),
    started_(0),
    nextConnId_(1)    
{
    LOG_DEBUG<<"init tcpserver";
    // 当有新用户连接时，Acceptor类中绑定的acceptChannel_会有读事件发生执行handleRead()调用TcpServer::newConnection回调
    acceptor_->setNewConnectionCallback(
        std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{
    for(auto &item : connections_)
    {
        TcpConnectionPtr conn(item.second);
        // 把原始的智能指针复位 让栈空间的TcpConnectionPtr conn指向该对象 当conn出了其作用域 即可释放智能指针指向的对象
        item.second.reset();    
        // 销毁连接
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn));
    }
}

// 设置底层subloop的个数
void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}


// 开启服务器监听
void TcpServer::start()
{
    if (started_++ == 0)
    {
        // 启动底层的lopp线程池
        threadPool_->start(threadInitCallback_);
        LOG_DEBUG<<"启动底层的loop线程池";
        // acceptor_.get()绑定时候需要地址
        // acceptor_.get()->listen 实际上bind绑定后 listen执行的顺序
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

// 有一个新用户连接，acceptor会执行这个回调操作，负责将mainLoop接收到的请求连接(acceptChannel_会有读事件发生)通过回调轮询分发给subLoop去处理
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    LOG_DEBUG<<"newConnection peerAdder"<<peerAddr.toIpPort();
    // 轮询算法 选择一个subLoop 来管理connfd对应的channel
    EventLoop *ioLoop = threadPool_->getNextLoop();
    // 提示信息
    char buf[64] = {0};
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    // 这里没有设置为原子类是因为其只在mainloop中执行 不涉及线程安全问题
    ++nextConnId_;   
    // 新连接名字
    std::string connName = name_ + buf;

    LOG_INFO << "TcpServer::newConnection [" << name_.c_str() << "] - new connection [" << connName.c_str() << "] from " << peerAddr.toIpPort().c_str();
    
    // 通过sockfd获取其绑定的本机的ip地址和端口信息
    sockaddr_in local;
    ::memset(&local, 0, sizeof(local));
    socklen_t addrlen = sizeof(local);
    /**
     * local（通过getsockname函数填充）主要包含的是服务器本地用于与客户端通信的地址信息。
     * 这包括服务器自身的 IP 地址（如果服务器有多个网络接口，会是实际用于此连接的接口的 IP 地址）
     * 和服务器用于与客户端通信的端口号。
     * 这个端口号通常是服务器之前通过bind操作绑定的用于监听客户端连接请求的端口号。
     */
    if(::getsockname(sockfd, (sockaddr *)&local, &addrlen) < 0)
    {
        LOG_ERROR << "sockets::getLocalAddr() failed";
    }

    InetAddress localAddr(local);
    TcpConnectionPtr conn(new TcpConnection(ioLoop,
                                            connName,
                                            sockfd,
                                            localAddr,
                                            peerAddr));
    /**
     * 所以说是每来一个连接，需要把这个连接所使用的loop还有一系列的东西封装成一个TcpConnectionPtr，然后将这个ptr放入到connections_中，
     * 这样的话，即使两个连接共用一个loop，也能通过TcpConnection中的channnel获得所对应的loop，
     * 这个时候就有一个常见的八股，就是tcp连接可以共用一个端口吗，可以用因为是通过四元组确定一个连接的，
     * 这个例子就是，它是通过的传入的ip和端口号，加上一个服务器端的 nextConnId_ 构建的 connName 来进行确定 tcp 连接。
    */
    connections_[connName] = conn;
    // 下面的回调都是用户设置给TcpServer => TcpConnection的，至于Channel绑定的则是TcpConnection设置的四个，
    // handleRead,handleWrite... 这下面的回调用于handlexxx函数中
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);//这个是把 Tcpserver 中的 messageCallback_ 与 TcpConnection 中的绑定
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setHighWaterMarkCallback(std::bind(&TcpServer::onHighWaterMark,this,std::placeholders::_1,std::placeholders::_2),conn->gethighWaterMark_());
    // 设置了如何关闭连接的回调 只是绑定回调
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));
    //这个是直接执行
    ioLoop->runInLoop(
        std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->runInLoop(
        std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}
void TcpServer::onHighWaterMark(const TcpConnectionPtr& conn, size_t len)
{
    LOG_DEBUG << "HighWaterMark " << len;
}
void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    LOG_DEBUG << "TcpServer::removeConnectionInLoop [" << name_.c_str() << "] - connection " << conn->name().c_str();

    connections_.erase(conn->name());
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn));
}