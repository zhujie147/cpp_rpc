#ifndef ROCKET_NET_EVENTLOOP_H
#define ROCKET_NET_EVENTLOOP_H
#include <pthread.h>
#include <set>
#include <functional>
#include <queue>
#include <errno.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <string.h>

#include "rocket/common/mutex.h"
#include "rocket/common/log.h"
#include "rocket/common/util.h"
#include "rocket/net/fd_event.h"
#include "rocket/net/wake_up_fd_event.h"
namespace rocket
{
    class EventLoop
    {
    public:
        EventLoop();
        ~EventLoop();
        void loop();

        void wakeup();

        void stop();

        void addEpollEvent(FdEvent* event);
        void deleteEpollEvent(FdEvent* event);

        bool isInLoopThread();

        void addTask(std::function<void()> cb,bool is_wake_up= false);
    private:
        void dealWakeup();
        void initWakeUpFdEvent();
        
    private:
        pid_t m_thread_id{0};
        int m_epoll_fd{0};
        int m_wakeup_fd{0};
        bool m_stop_flag{false};

        WakeUpFdEvent* m_wakeup_fd_event{NULL};

        std::set<int> m_listen_fds;
        // std::function<void()> 任何可以被当做函数调用的对象
        std::queue<std::function<void()>> m_pending_tasks;

        Mutex m_mutex;
    };
}

#endif