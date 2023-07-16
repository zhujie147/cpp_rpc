#include "rocket/net/eventloop.h"

#define ADD_TO_EPOLL()                                                                                                   \
    auto it = m_listen_fds.find(event->getFd());                                                                         \
    int op = EPOLL_CTL_ADD;                                                                                              \
    if (it != m_listen_fds.end())                                                                                        \
    {                                                                                                                    \
        op = EPOLL_CTL_MOD;                                                                                              \
    }                                                                                                                    \
    epoll_event tmp = event->getEpollEvent();                                                                            \
    INFOLOG("epoll_event.events = %d", (int)tmp.events);                                                                 \
    int rt = epoll_ctl(m_epoll_fd, op, event->getFd(), &tmp);                                                            \
    if (rt == -1)                                                                                                        \
    {                                                                                                                    \
        ERRORLOG("failed epoll_ctl when add fd %d, errno= %d, error info=%s\n", event->getFd(), errno, strerror(errno)); \
    }                                                                                                                    \
    DEBUGLOG("add event success, fd[%d]\n", event->getFd());

#define DELET_TO_EPOLL()                                                                                                 \
    auto it = m_listen_fds.find(event->getFd());                                                                         \
    int op = EPOLL_CTL_DEL;                                                                                              \
    if (it == m_listen_fds.end())                                                                                        \
    {                                                                                                                    \
        return;                                                                                                          \
    }                                                                                                                    \
    epoll_event tmp = event->getEpollEvent();                                                                            \
    int rt = epoll_ctl(m_epoll_fd, op, event->getFd(), &tmp);                                                            \
    if (rt == -1)                                                                                                        \
    {                                                                                                                    \
        ERRORLOG("failed epoll_ctl when add fd %d, errno= %d, error info=%s\n", event->getFd(), errno, strerror(errno)); \
    }                                                                                                                    \
    DEBUGLOG("delete event success, fd[%d]\n", event->getFd());

namespace rocket
{
    // 线程独有的变量
    static thread_local EventLoop *t_current_eventloop = NULL;
    static int g_epoll_max_timeout = 10000;
    static int g_epoll_max_events = 10;
    EventLoop::EventLoop()
    {
        if (t_current_eventloop != NULL)
        {
            ERRORLOG("failed to create event loop, this thread created event loop.\n");
            exit(0);
        }
        m_thread_id = getThreadId();
        m_epoll_fd = epoll_create(10);
        if (m_epoll_fd == -1)
        {
            ERRORLOG("failed to create event loop, epoll_create, error info[%s]\n", errno);
            exit(0);
        }
        initWakeUpFdEvent();
        INFOLOG("successfully create event loop int thread %d\n", m_thread_id);
        t_current_eventloop = this;
    }

    EventLoop::~EventLoop()
    {
        close(m_epoll_fd);
        if (m_wakeup_fd_event)
        {
            delete m_wakeup_fd_event;
            m_wakeup_fd_event = NULL;
        }
    }

    void EventLoop::initWakeUpFdEvent()
    {
        m_wakeup_fd = eventfd(0, EFD_NONBLOCK);
        if (m_wakeup_fd < 0)
        {
            ERRORLOG("failed to create event loop, eventfd, error info[%s]\n", errno);
            exit(0);
        }

        m_wakeup_fd_event = new WakeUpFdEvent(m_wakeup_fd);
        m_wakeup_fd_event->listen(FdEvent::IN_EVENT, [this]()
                                  {
            char buf[8];
            while (read(m_wakeup_fd, buf, 8) != -1 && errno != EAGAIN);
            DEBUGLOG("read full bytes from wakeup fd[%d]",m_wakeup_fd); });

        addEpollEvent(m_wakeup_fd_event);
    }

    void EventLoop::loop()
    {
        while (!m_stop_flag)
        {
            ScopeMutex<Mutex> lock(m_mutex);
            std::queue<std::function<void()>> tmp_tasks;
            m_pending_tasks.swap(tmp_tasks);
            lock.unlock();

            while (!tmp_tasks.empty())
            {
                std::function<void()> cb = tmp_tasks.front();
                tmp_tasks.pop();
                if (cb)
                {
                    cb();
                }
                else
                {
                    cb();
                }
            }

            int timeout = g_epoll_max_timeout;
            epoll_event result_events[g_epoll_max_events];
            // DEBUGLOG("now begin to epoll_wait");
            int rt = epoll_wait(m_epoll_fd, result_events, g_epoll_max_events, timeout);
            DEBUGLOG("now end epoll_wait, rt = %d", rt);
            if (rt < 0)
            {
                ERRORLOG("epoll_wait error, errorno=", errno);
            }
            else
            {
                for (int i = 0; i < rt; i++)
                {
                    epoll_event trigger_event = result_events[i];
                    FdEvent *fd_event = static_cast<FdEvent *>(trigger_event.data.ptr);
                    if (fd_event == NULL)
                    {
                        continue;
                    }
                    if (trigger_event.events & EPOLLIN)
                    {
                        DEBUGLOG("FD %d trigger EPOLLIN event",fd_event->getFd());
                        addTask(fd_event->handler(FdEvent::IN_EVENT));
                    }
                    if (trigger_event.events & EPOLLOUT)
                    {
                        DEBUGLOG("FD %d trigger EPOLLOUT event",fd_event->getFd());
                        addTask(fd_event->handler(FdEvent::OUT_EVENT));
                    }
                }
            }
        }
    }

    void EventLoop::wakeup()
    {
        m_wakeup_fd_event->wakeup();
    }

    void EventLoop::stop()
    {
        m_stop_flag = true;
    }

    void EventLoop::dealWakeup()
    {
    }

    void EventLoop::addEpollEvent(FdEvent *event)
    {
        if (isInLoopThread())
        {
            ADD_TO_EPOLL();
        }
        else
        {
            auto cb = [this, event]()
            {
                ADD_TO_EPOLL();
            };
            addTask(cb, true);
        }
    }
    void EventLoop::deleteEpollEvent(FdEvent *event)
    {
        if (isInLoopThread())
        {
            DELET_TO_EPOLL();
        }
        else
        {
            auto cb = [this, event]
            {
                DELET_TO_EPOLL();
            };
            addTask(cb, true);
        }
    }

    void EventLoop::addTask(std::function<void()> cb, bool is_wake_up /* =false */)
    {
        ScopeMutex<Mutex> lock(m_mutex);
        m_pending_tasks.push(cb);
        lock.unlock();
        if (is_wake_up)
        {
            wakeup();
        }
    }
    bool EventLoop::isInLoopThread()
    {
        return getThreadId() == m_thread_id;
    }

}