#include "rocket/net/tcp/tcp_connection.h"

namespace rocket
{
    TcpConnection::TcpConnection(IOThread *io_thread, int fd, int buffer_size, NetAddr::s_ptr peer_addr)
        : m_io_thread(io_thread), m_peer_addr(peer_addr), m_state(NotConnected), m_fd(fd)
    {
        m_in_buffer = std::make_shared<TcpBuffer>(buffer_size);
        m_out_buffer = std::make_shared<TcpBuffer>(buffer_size);

        m_fd_event = FdEventGroup::GetFdEventGroup()->getFdEvent(fd);
        m_fd_event->setNonBlock();
        m_fd_event->listen(FdEvent::IN_EVENT, std::bind(&TcpConnection::onRead, this));
        io_thread->getEventLoop()->addEpollEvent(m_fd_event);
    }
    TcpConnection::~TcpConnection()
    {
        DEBUGLOG("~tcpconnection");
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
        std::vector<char> tmp;
        int size = m_in_buffer->readAble();
        tmp.resize(size);
        m_in_buffer->readFromBuffer(tmp, size);

        std::string msg;
        for (int i = 0; i < tmp.size(); i++)
        {
            msg += tmp[i];
        }
        INFOLOG("success get request [%s] from client[%s]", msg.c_str(), m_peer_addr->toString().c_str());
        m_out_buffer->writeToBuffer(msg.c_str(), msg.size());
        m_fd_event->listen(FdEvent::OUT_EVENT, std::bind(&TcpConnection::onWrite, this));
        m_io_thread->getEventLoop()->addEpollEvent(m_fd_event);
    }
    void TcpConnection::onWrite()
    {
        if (m_state != Connected)
        {
            DEBUGLOG("onWrite error, client has already disconnected, addr[%s], clientfd[%d]", m_peer_addr->toString().c_str(), m_fd);
            return;
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
            m_io_thread->getEventLoop()->addEpollEvent(m_fd_event);
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
        m_io_thread->getEventLoop()->deleteEpollEvent(m_fd_event);
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
}