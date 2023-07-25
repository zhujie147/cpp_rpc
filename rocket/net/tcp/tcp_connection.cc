#include "rocket/net/tcp/tcp_connection.h"
#include "rocket/net/coder/string_coder.h"
namespace rocket
{
    TcpConnection::TcpConnection(EventLoop *event_loop, int fd, int buffer_size, NetAddr::s_ptr peer_addr,NetAddr::s_ptr local_addr, TcpConnectionType type /* = TcpConnectionByServer */)
        : m_event_loop(event_loop), m_peer_addr(peer_addr),m_local_addr(local_addr), m_state(NotConnected), m_fd(fd), m_connection_type(type)
    {
        m_in_buffer = std::make_shared<TcpBuffer>(buffer_size);
        m_out_buffer = std::make_shared<TcpBuffer>(buffer_size);

        m_fd_event = FdEventGroup::GetFdEventGroup()->getFdEvent(fd);
        m_fd_event->setNonBlock();
        if (m_connection_type == TcpConnectionByServer)
        {
            listenRead();
            m_dispatcher = std::make_shared<RpcDispatcher>();
        }

        m_coder = new TinyPBCoder();
    }
    TcpConnection::~TcpConnection()
    {
        DEBUGLOG("~tcpconnection");
        if (m_coder)
        {
            delete m_coder;
            m_coder = NULL;
        }
    }
    void TcpConnection::onRead()
    {
        if (m_state != Connected)
        {
            INFOLOG("client has already disconnnected, addr[%s], clientfd[%d]", m_peer_addr->toString(), m_fd);
            return;
        }

        bool is_read_all = false;
        bool is_close = false;
        while (!is_read_all)
        {
            if (m_in_buffer->writeAble() == 0)
            {
                m_in_buffer->resizeBuffer(2 * m_in_buffer->m_buffer.size());
            }
            int read_count = m_in_buffer->writeAble();
            int write_index = m_in_buffer->writeIndex();

            int rt = read(m_fd, &(m_in_buffer->m_buffer[write_index]), read_count);
            if (rt > 0)
            {
                m_in_buffer->moveWriteIndex(rt);
                if (rt == read_count)
                {
                    continue;
                }
                else if (rt < read_count)
                {
                    is_read_all = true;
                    break;
                }
            }
            else if (rt == 0)
            {
                is_close = true;
                break;
            }
            else if (rt == -1 && errno == EAGAIN)
            {
                is_read_all = true;
                break;
            }
        }

        if (is_close)
        {
            clear();
            INFOLOG("peer closed, peer addr [%d], clientfd[%d]", m_peer_addr->toString().c_str(), m_fd);
            return;
        }

        if (!is_read_all)
        {
            ERRORLOG("not read all data");
        }
        excute();
    }
    void TcpConnection::excute()
    {
        if (m_connection_type == TcpConnectionByServer)
        {
            std::vector<AbstractProtocol::s_ptr> result;
            std::vector<AbstractProtocol::s_ptr> replay_messages;
            m_coder->decode(result, m_in_buffer);
            for (size_t i = 0; i < result.size(); i++)
            {
                INFOLOG("success get request [%s] from client[%s]", result[i]->m_msg_id.c_str(), m_peer_addr->toString().c_str());
                std::shared_ptr<TinyPBProtocol> message = std::make_shared<TinyPBProtocol>();
                // message->m_pb_data="hello , this is rocket rpc test data";
                // message->m_msg_id=result[i]->m_msg_id;
                m_dispatcher->dispatcher(result[i], message,this);
                replay_messages.emplace_back(message);
            }
            // m_out_buffer->writeToBuffer(msg.c_str(), msg.size());
            m_coder->encode(replay_messages, m_out_buffer);
            listenWrite();
        }
        else
        {
            std::vector<AbstractProtocol::s_ptr> result;
            m_coder->decode(result, m_in_buffer);
            for (size_t i = 0; i < result.size(); i++)
            {
                std::string req_id = result[i]->m_msg_id;
                auto it = m_read_dones.find(req_id);
                if (it != m_read_dones.end())
                {
                    it->second(result[i]->shared_from_this());
                }
            }
        }
    }
    void TcpConnection::onWrite()
    {
        if (m_state != Connected)
        {
            DEBUGLOG("onWrite error, client has already disconnected, addr[%s], clientfd[%d]", m_peer_addr->toString().c_str(), m_fd);
            return;
        }

        if (m_connection_type == TcpConnectionByClient)
        {
            std::vector<AbstractProtocol::s_ptr> messages;
            for (size_t i = 0; i < m_write_dones.size(); i++)
            {
                messages.push_back(m_write_dones[i].first);
            }
            m_coder->encode(messages, m_out_buffer);
        }

        bool is_write_all = false;
        while (true)
        {
            if (m_out_buffer->readAble() == 0)
            {
                DEBUGLOG("no data need to send to client [%s]", m_peer_addr->toString().c_str());
                is_write_all = true;
                break;
            }
            int write_size = m_out_buffer->readAble();
            int read_index = m_out_buffer->readIndex();

            int rt = write(m_fd, &(m_out_buffer->m_buffer[read_index]), write_size);

            if (rt >= write_size)
            {
                DEBUGLOG("no data need to send to client [%s]", m_peer_addr->toString().c_str());
                is_write_all = true;
                break;
            }
            else if (rt == -1 && errno == EAGAIN)
            {
                ERRORLOG("write data error, errno == EAGIN and rt ==-1");
                break;
            }
        }
        if (is_write_all)
        {
            m_fd_event->cancle(FdEvent::OUT_EVENT);
            m_event_loop->addEpollEvent(m_fd_event);
        }

        if (m_connection_type == TcpConnectionByClient)
        {
            for (size_t i = 0; i < m_write_dones.size(); i++)
            {
                m_write_dones[i].second(m_write_dones[i].first);
            }
            m_write_dones.clear();
        }
    }
    void TcpConnection::setState(const TcpState state)
    {
        m_state = state;
    }
    TcpState TcpConnection::getState()
    {
        return m_state;
    }
    void TcpConnection::clear()
    {
        if (m_state == Closed)
        {
            return;
        }
        m_fd_event->cancle(FdEvent::IN_EVENT);
        m_fd_event->cancle(FdEvent::OUT_EVENT);
        m_event_loop->deleteEpollEvent(m_fd_event);
        m_state = Closed;
    }
    void TcpConnection::shutdown()
    {
        if (m_state == Closed || m_state == NotConnected)
        {
            return;
        }
        m_state = HalfClosing;
        // 发送FIN报文
        ::shutdown(m_fd, SHUT_RDWR);
    }

    void TcpConnection::setConnectionType(TcpConnectionType type)
    {
        m_connection_type = type;
    }

    void TcpConnection::listenWrite()
    {
        m_fd_event->listen(FdEvent::OUT_EVENT, std::bind(&TcpConnection::onWrite, this));
        m_event_loop->addEpollEvent(m_fd_event);
    }

    void TcpConnection::listenRead()
    {
        m_fd_event->listen(FdEvent::IN_EVENT, std::bind(&TcpConnection::onRead, this));
        m_event_loop->addEpollEvent(m_fd_event);
    }

    void TcpConnection::pushSendMessage(AbstractProtocol::s_ptr message, std::function<void(AbstractProtocol::s_ptr)> done)
    {
        m_write_dones.push_back(std::make_pair(message, done));
    }

    void TcpConnection::pushReadMessage(const std::string &req_id, std::function<void(AbstractProtocol::s_ptr)> done)
    {
        m_read_dones.insert(std::make_pair(req_id, done));
    }

    NetAddr::s_ptr TcpConnection::getLocalAddr()
    {
        return m_local_addr;
    }
    NetAddr::s_ptr TcpConnection::getPeerAddr()
    {
        return m_peer_addr;
    }
}