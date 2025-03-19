#include <functional>
#include <string>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>

#include "TcpConnection.h"
#include "Logging.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    // 如果传入EventLoop没有指向有意义的地址则出错
    // 正常来说在 TcpServer::start 这里就生成了新线程和对应的EventLoop
    if (loop == nullptr)
    {
         LOG_ERROR << "mainLoop is null!";
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop,
                             const std::string &nameArg,
                             int sockfd,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : loop_(CheckLoopNotNull(loop))
    , name_(nameArg)
    , state_(kConnecting)
    , reading_(true)
    , socket_(new Socket(sockfd))
    , channel_(new Channel(loop, sockfd))
    , localAddr_(localAddr)
    , peerAddr_(peerAddr)
    , highWaterMark_(64 * 1024 * 1024) // 64M 避免发送太快对方接受太慢
{
    // 下面给channel设置相应的回调函数 poller给channel通知感兴趣的事件发生了 channel会回调相应的回调函数 这个是TcpConnection中的channel
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(
        std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(
        std::bind(&TcpConnection::handleError, this));

    LOG_INFO << "TcpConnection::ctor[" << name_.c_str() << "] at fd =" << sockfd;
    socket_->setKeepAlive(true);
    // socket_->setKeepAlive(false);//这个是为了压测
}

TcpConnection::~TcpConnection()
{
    LOG_DEBUG << "TcpConnection::dtor[" << name_.c_str() << "] at fd=" << channel_->fd() << " state=" << static_cast<int>(state_);
}


// 发送数据
void TcpConnection::send(const std::string &buf)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(buf.c_str(), buf.size());
            LOG_DEBUG<<"send: run in loop";
        }
        else
        {
            // 遇到重载函数的绑定，可以使用函数指针来指定确切的函数
            void(TcpConnection::*fp)(const void* data, size_t len) = &TcpConnection::sendInLoop;
            loop_->runInLoop(std::bind(
                fp,
                shared_from_this(),
                buf.c_str(),
                buf.size()));
            LOG_DEBUG<<"send: not run in loop";
        }
    }
}

void TcpConnection::send(Buffer *buf)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(buf->peek(), buf->readableBytes());
            buf->retrieveAll();
            // LOG_DEBUG<<"send: run in loop";
        }
        else
        {
            // sendInLoop有多重重载，需要使用函数指针确定
            void (TcpConnection::*fp)(const std::string& message) = &TcpConnection::sendInLoop;
            // loop_->runInLoop(std::bind(fp, this, buf->retrieveAllAsString()));
            loop_->runInLoop(std::bind(fp, shared_from_this(), buf->retrieveAllAsString()));
            LOG_DEBUG<<"send: not run in loop";
        }
    }
}

void TcpConnection::sendInLoop(const std::string& message)
{
    sendInLoop(message.data(), message.size());
}

/**
 * 发送数据 应用写的快 而内核发送数据慢 需要把待发送数据写入缓冲区，而且设置了水位回调
 **/
void TcpConnection::sendInLoop(const void* data, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    // 之前调用过connection得shutdown，不能再进行发送了
    if (state_ == kDisconnected)
    {
        LOG_ERROR << "disconnected, give up writing";
        return;
    }

    // channel第一次写数据，且缓冲区没有待发送数据
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = ::write(channel_->fd(), data, len);
        LOG_DEBUG << channel_->fd() <<" write num: "<<nwrote<<"\tsize:"<<len;
        if (nwrote >= 0)
        {
            // 判断有没有一次性写完
            remaining = len - nwrote;
            LOG_DEBUG<<"remain: "<<remaining;
            if (remaining == 0 && writeCompleteCallback_)
            {
                // 既然一次性发送完事件就不用让channel对epollout事件感兴趣了
                loop_->queueInLoop(
                    std::bind(writeCompleteCallback_, shared_from_this())
                );
            }
        }
        else // nwrote = 0
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                
                if (errno == EPIPE || errno == ECONNRESET) // SIGPIPE
                {
                    faultError = true;
                }
            }
        }

    }

    // 说明一次性并没有发送完数据，剩余数据需要保存到缓冲区中，且需要改channel注册写事件
    if (!faultError && remaining > 0)
    {
        LOG_DEBUG<<"without send";
        size_t oldLen = outputBuffer_.readableBytes();
        void (TcpConnection::*fp)() = &TcpConnection::highWaterMarkInLoop;

        // 其实我觉得这个位置设置高水位的目的，并不是 没有发送完存起来下次发送，而是保存起来要不要再发送，或者直接结束。
        // 如果小于水位，即使有未发送完的，下次再发送就好了。
        if (oldLen + remaining >= highWaterMark_ 
        && oldLen < highWaterMark_ 
        && fp)
        {
            // TODO
            loop_->queueInLoop(std::bind(
                fp, shared_from_this()));
                LOG_DEBUG<<"highWaterMarkCallback_";
        }
        outputBuffer_.append((char *)data + nwrote, remaining);
        if (!channel_->isWriting())
        {
             // 这里一定要注册channel的写事件 否则poller不会给channel通知epollout 
             // 这个通知后，channel通过一系列回调会执行TcpConnection::handleWrite(){}
            channel_->enableWriting();
        }
    }
}
void TcpConnection::highWaterMarkInLoop(){
    // 其实这个里边应该是想办法让内核降速，因为保存到outPutBuffer中的数据是由channel自己发送的
    LOG_DEBUG<<"exceed the highWaterMark_ and need channel to send message. "; 
}
// 关闭连接 
void TcpConnection::shutdown()
{
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(
            std::bind(&TcpConnection::shutdownInLoop, shared_from_this()));
        LOG_DEBUG<<"shutdownInLoop";
    }
}

void TcpConnection::shutdownInLoop()
{
    if (!channel_->isWriting()) // 说明当前outputBuffer_的数据全部向外发送完成
    {
        socket_->shutdownWrite();
        LOG_DEBUG<<"shutdownInLoop";
    }
}

// 连接建立
void TcpConnection::connectEstablished()
{
    setState(kConnected); // 建立连接，设置一开始状态为连接态
    /**
     * TODO:tie
     * channel_->tie(shared_from_this());
     * tie相当于在底层有一个强引用指针记录着，防止析构
     * 为了防止TcpConnection这个资源被误删掉，而这个时候还有许多事件要处理
     * channel->tie 会进行一次判断，是否将弱引用指针变成强引用，变成得话就防止了计数为0而被析构得可能
     */
    channel_->tie(shared_from_this());
    channel_->enableReading(); // 向poller注册channel的EPOLLIN读事件

    // 新连接建立 执行回调
    connectionCallback_(shared_from_this());
}

// 连接销毁
void TcpConnection::connectDestroyed()
{
    if (state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll(); // 把channel的所有感兴趣的事件从poller中删除掉
        connectionCallback_(shared_from_this()); //这个会走到
    }
    channel_->remove(); // 把channel从poller中删除掉
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0;
    // TcpConnection会从socket读取数据，然后写入inpuBuffer
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if (n > 0)
    {
        // 已建立连接的用户，有可读事件发生，调用用户传入的回调操作 HttpServer::onMessage
        // TODO:shared_from_this
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0)
    {
        // 没有数据，说明客户端关闭连接
        handleClose();
    }
    else
    {
        // 出错情况
        errno = savedErrno;
        LOG_ERROR << "TcpConnection::handleRead() failed";
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if (channel_->isWriting())
    {
        int saveErrno = 0;
        // 这个 channel_->fd() 除了acceptchannel的fd是socketfd之外，其他的channel的fd都是对端的fd，通过accept4生成的
        // 经 NewConnectionCallback_(connfd, peerAddr); 传递的
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &saveErrno);
        // 正确读取数据
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            // 说明buffer可读数据都被TcpConnection读取完毕并写入给了客户端
            // 此时就可以关闭连接，否则还需继续提醒写事件
            if (outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWriting();
                // 调用用户自定义的写完数据处理函数
                if (writeCompleteCallback_)
                {
                    // 唤醒loop_对应得thread线程，执行写完成事件回调
                    loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_DEBUG << "TcpConnection::handleWrite() failed";
        }
    }
    // state_不为写状态
    else
    {
        LOG_ERROR << "TcpConnection fd=" << channel_->fd() << " is down, no more writing";
    }
}

void TcpConnection::handleClose()
{
    setState(kDisconnected);    // 设置状态为关闭连接状态
    channel_->disableAll();     // 注销Channel所有感兴趣事件
    
    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr);   
    closeCallback_(connPtr);        // 关闭连接得回调
}

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof(optval);
    int err = 0;
    // TODO:getsockopt ERROR
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen))
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR << "cpConnection::handleError name:" << name_.c_str() << " - SO_ERROR:" << err;
}