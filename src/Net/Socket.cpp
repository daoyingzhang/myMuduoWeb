#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#include "Socket.h"
#include "Logging.h"
#include "InetAddress.h"

Socket::~Socket()
{
    ::close(sockfd_);
}

void Socket::bindAddress(const InetAddress &localaddr)
{
    /**
     * 在网络编程中，服务器通常需要明确地绑定到一个特定的地址和端口，以便客户端能够准确地连接到服务器。
     * 通过调用bind函数，服务器将指定的套接字与特定的本地地址和端口关联起来，
     * 这样当客户端向这个地址和端口发送连接请求时，操作系统就能够将请求正确地路由到这个套接字上。
     */
    if (0 != ::bind(sockfd_, (sockaddr *)localaddr.getSockAddr(), sizeof(sockaddr_in)))
    {
        LOG_FATAL << "bind sockfd:" << sockfd_ << " fail";
    }
}

void Socket::listen()
{
    if (0 != ::listen(sockfd_, 1024))
    {
        LOG_FATAL << "listen sockfd:" << sockfd_ << " fail";
    }
}

int Socket::accept(InetAddress *peeraddr)
{
    /**
     * 1. accept函数的参数不合法
     * 2. 对返回的connfd没有设置非阻塞
     * Reactor模型 one loop per thread
     * poller + non-blocking IO
     **/
    sockaddr_in addr;
    socklen_t len = sizeof(addr);
    ::memset(&addr, 0, sizeof(addr));
    /**
     * 如果成功接受了连接请求，accept4 返回一个新的套接字描述符，代表与客户端的连接，这个描述符被赋值给 connfd。
     * 如果出现错误，例如没有连接请求或者发生其他错误，accept4 返回 -1，可以通过检查 errno 变量来确定具体的错误原因。
     * 同时，会将客户端的地址信息会被放入addr中
     * 
     * SOCK_NONBLOCK ： 
     *     表示非阻塞模式（Non-blocking Mode） 在非阻塞模式下，
     *     I/O 操作（如 read()、write()、accept() 等）在无法立即完成时会立刻返回，而不是阻塞等待数据。
     * SOCK_CLOEXEC ： 
     *     表示在执行 exec 系列函数时自动关闭该文件描述符（Close-on-Exec）
     *     防止子进程继承该文件描述符，提升安全性。
     *     常用于多线程或多进程环境，以避免意外的文件描述符泄漏。
     */
    int connfd = ::accept4(sockfd_, (sockaddr *)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    // int connfd = ::accept(sockfd_, (sockaddr *)&addr, &len);
    if (connfd >= 0)
    {
        //提供一个能够调用刚刚存储的客户端的addr，将其保存到一个addr_全局变量上（他在InetAddress.h里，）
        peeraddr->setSockAddr(addr);
        LOG_INFO<<"setSockAddr successed !";
    }
    else
    {
        LOG_ERROR << "accept4() failed";
    }
    return connfd;
}

// TODO:socket的各个设置的用法
void Socket::shutdownWrite()
{
    /**
     * 关闭写端，但是可以接受客户端数据
     */
    if (::shutdown(sockfd_, SHUT_WR) < 0)
    {
        LOG_ERROR << "shutdownWrite error";
    }
}

// 不启动Nagle算法
void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)); // TCP_NODELAY包含头文件 <netinet/tcp.h>
}

// 设置地址复用，其实就是可以使用处于Time-wait的端口
void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)); // TCP_NODELAY包含头文件 <netinet/tcp.h>
}

// 通过改变内核信息，多个进程可以绑定同一个地址。通俗就是多个服务的ip+port是一样
void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)); // TCP_NODELAY包含头文件 <netinet/tcp.h>
}

void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)); // TCP_NODELAY包含头文件 <netinet/tcp.h>
}