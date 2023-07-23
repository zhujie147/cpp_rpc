#ifndef ROCKET_NET_TCP_TCP_CONNECTION_H
#define ROCKET_NET_TCP_TCP_CONNECTION_H

#include <unistd.h>
#include <memory>
#include <string.h>
#include <map>
#include <queue>
#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/tcp/tcp_buffer.h"
#include "rocket/common/log.h"
#include "rocket/net/fd_event_group.h"
#include "rocket/net/io_thread.h"
#include "rocket/net/abstract_coder.h"
namespace rocket
{
    enum TcpState
    {
        NotConnected = 1,
        Connected = 2,
        HalfClosing = 3,
        Closed = 4,
    };

    enum TcpConnectionType
    {
        TcpConnectionByServer = 1,
        TcpConnectionByClient = 2,
    };
    class TcpConnection
    {
    public:
        typedef std::shared_ptr<TcpConnection> s_ptr;

    public:
        TcpConnection(EventLoop *event_loop, int fd, int buffer_size, NetAddr::s_ptr peer_addr, TcpConnectionType type = TcpConnectionByServer);
        ~TcpConnection();
        void onRead();
        void excute();
        void onWrite();
        void setState(const TcpState state);
        TcpState getState();
        void clear();
        void shutdown();
        void setConnectionType(TcpConnectionType type);

        void listenWrite();
        void listenRead();

        void pushSendMessage(AbstractProtocol::s_ptr message, std::function<void(AbstractProtocol::s_ptr)> done);
        void pushReadMessage(const std::string& req_id, std::function<void(AbstractProtocol::s_ptr)> done);

    private:
        EventLoop *m_event_loop{NULL};
        NetAddr::s_ptr m_local_addr;
        NetAddr::s_ptr m_peer_addr;

        TcpBuffer::s_ptr m_in_buffer;
        TcpBuffer::s_ptr m_out_buffer;
        TcpState m_state;
        FdEvent *m_fd_event{NULL};
        AbstractCoder *m_coder{NULL};

        int m_fd{0};

        TcpConnectionType m_connection_type{TcpConnectionByServer};
        std::vector<std::pair<AbstractProtocol::s_ptr, std::function<void(AbstractProtocol::s_ptr)>>> m_write_dones;
        std::map<std::string, std::function<void(AbstractProtocol::s_ptr)>> m_read_dones;
    };
}
#endif