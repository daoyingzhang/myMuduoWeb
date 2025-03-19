#include "httpServer.h"
#include "httpRequest.h"
#include "httpResponse.h"
#include "Timestamp.h"
#include "AsyncLogging.h"
int kRollSize = 500*1000*1000;

// 异步日志
std::unique_ptr<AsyncLogging> g_asyncLog;

void asyncOutput(const char* msg, int len)
{
    g_asyncLog->append(msg, len);
}

void setLogging(const char* argv0)
{
    Logger::setOutput(asyncOutput);
    char name[256];
    strncpy(name, argv0, 256);
    // std::cout<<name<<std::endl;
    g_asyncLog.reset(new AsyncLogging(::basename(name), kRollSize));
    Logger::setLogLevel(Logger::DEBUG);
    g_asyncLog->start();
}


extern char favicon[555];
bool benchmark = false;

int main(int argc, char* argv[])
{
    setLogging(argv[0]);
    LOG_INFO << "pid = " << getpid();
    
    EventLoop loop;
    HttpServer server(&loop, InetAddress(8080), std::string("http-server"),10,std::string("ming")
    ,std::string("111111"),std::string("ming_database"),std::string("localHost"));
    server.start();
    loop.loop();
}
