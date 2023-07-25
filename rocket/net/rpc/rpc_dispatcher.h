#ifndef ROCKET_NET_RPC_RPC_DISPATCHER_H
#define ROCKET_NET_RPC_RPC_DISPATCHER_H

#include <map>
#include <memory>
#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include "rocket/net/coder/abstract_protocol.h"
#include "rocket/common/log.h"
#include "rocket/net/rpc/rpc_controller.h"
#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/tcp/tcp_connection.h"
#include "rocket/net/coder/tinypb_coder.h"
#include "rocket/net/coder/abstract_protocol.h"
namespace rocket
{
    class TcpConnection;
    class RpcDispatcher
    {
    public:
        typedef std::shared_ptr<google::protobuf::Service> service_s_ptr;
        void dispatcher(AbstractProtocol::s_ptr request, AbstractProtocol::s_ptr response,TcpConnection* connection);

        void registerService(service_s_ptr service);
        void setTinyPBError(std::shared_ptr<TinyPBProtocol>msg, int32_t err_code,const std::string err_info);
    private:
        bool parseServiceFullName(const std::string& full_name, std::string& service_name, std::string& method_name);

    private:
        std::map<std::string,service_s_ptr> m_service_map;
    };
}

#endif