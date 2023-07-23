#include <assert.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <memory>
#include <string>
#include "rocket/common/log.h"
#include "rocket/common/config.h"

void tests_connect()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        ERRORLOG("invalid fd %d", fd);
        exit(0);
    }

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    inet_aton("127.0.0.1", &server_addr.sin_addr);

    int rt = connect(fd, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr));
    std::string msg = "hello rocket";
    rt = write(fd, msg.c_str(), msg.length());
    DEBUGLOG("success write %d bytes, [%s]",rt,msg.c_str());
    char buf[100];
    memset(buf,0,sizeof(buf));
    rt=read(fd,buf,100);
    DEBUGLOG("success read %d bytes, [%s]",rt,std::string(buf).c_str());
    while(1);
}

int main()
{

    rocket::Config::SetGlobalConfig("./conf/rocket.xml");

    rocket::Logger::InitGlobalLogger();
    tests_connect();
    return 0;
}