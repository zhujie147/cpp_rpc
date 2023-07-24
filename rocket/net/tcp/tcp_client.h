#ifndef ROCKET_NET_TCP_TCP_CLIENT_H
#define ROCKET_NET_TCP_TCP_CLIENT_H

#include <sys/socket.h>
#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/eventloop.h"
#include "rocket/net/tcp/tcp_connection.h"
#include "rocket/net/coder/abstract_protocol.h"
#include "rocket/common/log.h"
#include "rocket/net/fd_event_group.h"
namespace rocket{
    class TcpClient{
        public:
            TcpClient(NetAddr::s_ptr peer_addr);
            ~TcpClient();

            void connect(std::function<void()> done);
            void writeMessage(AbstractProtocol::s_ptr message,std::function<void(AbstractProtocol::s_ptr)> done);
            void readMessage(const std::string& req_id,std::function<void(AbstractProtocol::s_ptr)> done);
        private:
            NetAddr::s_ptr m_peer_addr;
            EventLoop* m_event_loop{NULL};
            FdEvent* m_fd_event {NULL};
            

            int m_fd{-1};
            TcpConnection::s_ptr m_connection;
    };
}


#endif