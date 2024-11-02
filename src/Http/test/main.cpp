#include "httpServer.h"
#include "httpRequest.h"
#include "httpResponse.h"
#include "Timestamp.h"

extern char favicon[555];
bool benchmark = false;

int main(int argc, char* argv[])
{
    EventLoop loop;
    HttpServer server(&loop, InetAddress(8080), std::string("http-server"),10,std::string("ming")
    ,std::string("111111"),std::string("ming_database"),std::string("localHost"));
    server.start();
    loop.loop();
}
