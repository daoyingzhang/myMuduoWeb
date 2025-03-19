#pragma once

/**
 * 它用于mainLoop中，
 * Acceptor用于接受新用户连接并分发连接给SubReactor（Sub EventLoop），
 * 封装了服务器监听套接字fd以及相关处理方法。
 * Acceptor类内部其实没有贡献什么核心的处理函数，主要是对其他类的方法调用进行封装。
 * 但是Acceptor中核心的点在于将监听套接字和Channel绑定在了一起，并为其注册了读回调handRead。
 * 当有连接到来的时候，读事件发生，就会调用相应读回调handleRead，读回调会间接调用TcpServer的newConnection()，
 * 该函数负责以轮询的方式把连接分发给sub EventLoop去处理。
 */
#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

class EventLoop;
class InetAddress;

/**
 * Acceptor运行在mainLoop中
 * TcpServer发现Acceptor有一个新连接，则将此channel分发给一个subLoop
 */
class Acceptor
{
public:
    // 接受新连接的回调函数
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress&)>;
    Acceptor(EventLoop *loop, const InetAddress &ListenAddr, bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback &cb)
    {
        NewConnectionCallback_ = cb;
    }

    bool listenning() const { return listenning_; }
    void listen(); //这个实际上是调用的Socket类中的监听

private:
    void handleRead();

    EventLoop *loop_; // Acceptor用的就是用户定义的BaseLoop
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback NewConnectionCallback_;
    bool listenning_; // 是否正在监听的标志
};
