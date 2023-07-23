#ifndef ROCKET_NET_TCP_TCP_CONNECTION_H
#define ROCKET_NET_TCP_TCP_CONNECTION_H

#include <unistd.h>
#include <memory>
#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/tcp/tcp_buffer.h"
#include "rocket/common/log.h"
#include "rocket/net/fd_event_group.h"
#include "rocket/net/io_thread.h"
namespace rocket
{
    enum TcpState
    {
        NotConnected = 1,
        Connected = 2,
        HalfClosing = 3,
        Closed = 4,
    };
    class TcpConnection
    {
    public:
        typedef std::shared_ptr<TcpConnection> s_ptr;

    public:
        TcpConnection(IOThread *io_thread, int fd, int buffer_size, NetAddr::s_ptr peer_addr);
        ~TcpConnection();
        void onRead();
        void excute();
        void onWrite();
        void setState(const TcpState state);
        TcpState getState();
        void clear();
        void shutdown();

    private:
        IOThread *m_io_thread{NULL};
        NetAddr::s_ptr m_local_addr;
        NetAddr::s_ptr m_peer_addr;

        TcpBuffer::s_ptr m_in_buffer;
        TcpBuffer::s_ptr m_out_buffer;
        TcpState m_state;
        FdEvent *m_fd_event{NULL};
        int m_fd{0};
        
    };
}
#endif