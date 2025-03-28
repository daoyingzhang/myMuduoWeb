#pragma once


/**
 * 这个类主要封装了一个已建立的TCP连接，以及控制该TCP连接的方法（连接建立和关闭和销毁），
 * 以及该连接发生的各种事件（读/写/错误/连接）对应的处理函数，
 * 以及这个TCP连接的服务端和客户端的套接字地址信息等。
 */


#include <memory>
#include <string>
#include <atomic>
#include <string>

#include "noncopyable.h"
#include "Callback.h"
#include "Buffer.h"
#include "Timestamp.h"
#include "InetAddress.h"
#include "Logging.h"
class Channel;
class EventLoop;
class Socket;

class TcpConnection : noncopyable, 
    public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop *loop,
                const std::string &nameArg,
                int sockfd,
                const InetAddress &localAddr,
                const InetAddress &peerAddr);
    ~TcpConnection();

    EventLoop* getLoop() const { return loop_; }
    const std::string& name() const { return name_; }
    const InetAddress& localAddress() const { return localAddr_; }
    const InetAddress& peerAddress() const { return peerAddr_; }

    bool connected() const { return state_ == kConnected; }

    // 发送数据
    void send(const std::string &buf);
    void send(Buffer *buf);

    // 关闭连接
    void shutdown();

    // 保存用户自定义的回调函数
    void setConnectionCallback(const ConnectionCallback &cb){ 
        connectionCallback_ = cb; 
        LOG_DEBUG<<"setConnectionCallback";
    }
    void setMessageCallback(const MessageCallback &cb){
         messageCallback_ = cb; 
         LOG_DEBUG<<"setMessageCallback";
    }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb){ 
        writeCompleteCallback_ = cb;
         LOG_DEBUG<<"setWriteCompleteCallback"; 
    }
    void setCloseCallback(const CloseCallback &cb){ 
        closeCallback_ = cb; 
        LOG_DEBUG<<"setCloseCallback"; 
    }
    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark){
         highWaterMarkCallback_ = cb; 
         highWaterMark_ = highWaterMark;  
         LOG_DEBUG<<"setHighWaterMarkCallback";
    }
    
    // TcpServer会调用
    void connectEstablished(); // 连接建立
    void connectDestroyed();   // 连接销毁
    size_t gethighWaterMark_(){return highWaterMark_;}
    // @Tip：
    //这个位置原来是私有得
    std::unique_ptr<Socket> socket_;
private:
    enum StateE
    {
        kDisconnected, // 已经断开连接
        kConnecting,   // 正在连接
        kConnected,    // 已连接
        kDisconnecting // 正在断开连接
    };    
    void setState(StateE state) { state_ = state; }

    // 注册到channel上的回调函数，poller通知后会调用这些函数处理
    // 然后这些函数最后会再调用从用户那里传来的回调函数
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void* message, size_t len);
    void sendInLoop(const std::string& message);
    void shutdownInLoop();
    void highWaterMarkInLoop();
    EventLoop *loop_;           // 属于哪个subLoop（如果是单线程则为mainLoop）
    const std::string name_;
    std::atomic_int state_;     // 连接状态
    bool reading_;

    
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;   // 本服务器地址
    const InetAddress peerAddr_;    // 对端地址

    /**
     * 用户自定义的这些事件的处理函数，然后传递给 TcpServer 
     * TcpServer 再在创建 TcpConnection 对象时候设置这些回调函数到 TcpConnection中
     */
    ConnectionCallback connectionCallback_;         // 有新连接时的回调
    MessageCallback messageCallback_;               // 有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_;   // 消息发送完成以后的回调
    CloseCallback closeCallback_;                   // 客户端关闭连接的回调
    HighWaterMarkCallback highWaterMarkCallback_;   // 超出水位实现的回调
    size_t highWaterMark_;

    Buffer inputBuffer_;    // 读取数据的缓冲区
    Buffer outputBuffer_;   // 发送数据的缓冲区
};