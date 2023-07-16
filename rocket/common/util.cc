#include "common/util.h"
#include <sys/syscall.h>
namespace rocket
{

    static int g_pid = 0;
    //可以使用该变量只属于访问它的线程，且每个能访问它的线程都会单独为该变量分配内存
    static thread_local int g_thread_id = 0;
    pid_t getPid()
    {
        if(g_pid!=0){
            return g_pid;
        }
        return getpid();
    }

    pid_t getThreadId()
    {
        if(g_thread_id!=0){
            return g_thread_id;
        }
        return syscall(SYS_gettid);
    }
}