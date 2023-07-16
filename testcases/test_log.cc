#include <pthread.h>
#include <rocket/common/log.h>
#include <rocket/common/config.h>

int main(){
    rocket::Config::SetGlobalConfig("./conf/rocket.xml");
    rocket::Logger::InitGlobalLogger();
    // pthread_t thread;
    // pthread_create(&thread,NULL,&fun,NULL);
    int i=20;
    while(i--){
        DEBUGLOG("test debug log %s","11");
        INFOLOG("test info log %s","11");
    }
    
    return 0;
}