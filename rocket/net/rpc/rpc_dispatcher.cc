#include "rocket/net/coder/tinypb_coder.h"
#include "rocket/net/rpc/rpc_dispatcher.h"
#include "rocket/common/error_code.h"
namespace rocket
{

    void RpcDispatcher::dispatcher(AbstractProtocol::s_ptr request, AbstractProtocol::s_ptr response,TcpConnection* connection)
    {
        std::shared_ptr<TinyPBProtocol> req_protocol = std::dynamic_pointer_cast<TinyPBProtocol>(request);
        std::shared_ptr<TinyPBProtocol> rsp_protocol = std::dynamic_pointer_cast<TinyPBProtocol>(response);
        std::string method_full_name = req_protocol->m_method_name;
        std::string service_name;
        std::string method_name;
        rsp_protocol->m_msg_id = req_protocol->m_msg_id;
        rsp_protocol->m_method_name = req_protocol->m_method_name;

        if (!parseServiceFullName(method_full_name, service_name, method_name))
        {
            setTinyPBError(rsp_protocol, ERROR_PARSE_SERVICE_NAME, "parse service name error");
            return;
        }

        auto it = m_service_map.find(service_name);
        if (it == m_service_map.end())
        {
            ERRORLOG("%s | service name[%s] not found",req_protocol->m_msg_id.c_str(),service_name.c_str());
            setTinyPBError(rsp_protocol, ERROR_SERVICE_NOT_FOUND, "service not found");
            return;
        }

        service_s_ptr service = (*it).second;
        const google::protobuf::MethodDescriptor *method = service->GetDescriptor()->FindMethodByName(method_name);
        if (method == NULL)
        {
            ERRORLOG("%s | method name[%s] not found in service[%s]",req_protocol->m_msg_id.c_str(),method_name.c_str(),service_name.c_str());
            setTinyPBError(rsp_protocol, ERROR_SERVICE_NOT_FOUND, "method not found");
            return;
        }

        google::protobuf::Message *req_msg = service->GetRequestPrototype(method).New();

        if (!req_msg->ParseFromString(req_protocol->m_pb_data))
        {
            ERRORLOG("%s | deserilize error",req_protocol->m_msg_id.c_str());
            setTinyPBError(rsp_protocol, ERROR_FAILED_DESERIALIZE, "deserilize error");
            if(req_msg!=NULL){
                delete req_msg;
                req_msg=NULL;
            }
            return;
        }
        INFOLOG("%s | get rpc request[%s]", req_protocol->m_msg_id.c_str(), req_msg->ShortDebugString().c_str());

        google::protobuf::Message *rsp_msg = service->GetResponsePrototype(method).New();


        RpcController rpcController;
        rpcController.SetLocalAddr(connection->getLocalAddr());
        rpcController.SetPeerAddr(connection->getPeerAddr());
        rpcController.SetReqId(req_protocol->m_msg_id);
        service->CallMethod(method, &rpcController, req_msg,rsp_msg,NULL);


        if(!rsp_msg->SerializePartialToString(&(rsp_protocol->m_pb_data))){
            ERRORLOG("%s | serialize error, origin message [%s]",req_protocol->m_msg_id.c_str(),rsp_msg->ShortDebugString().c_str());
            setTinyPBError(rsp_protocol, ERROR_SERVICE_NOT_FOUND, "serilize error");
            if(rsp_msg!=NULL){
                delete rsp_msg;
                rsp_msg=NULL;
            }
             if(req_msg!=NULL){
                delete req_msg;
             }
            return;
        }

        rsp_protocol->m_err_code = 0;
        INFOLOG("%s | dispatch success, request[%s], response[%s]",req_protocol->m_msg_id.c_str(),req_msg->ShortDebugString().c_str(),rsp_msg->ShortDebugString().c_str());
        delete rsp_msg;
        delete req_msg;
        rsp_msg=NULL;
        req_msg=NULL;
    }

    bool RpcDispatcher::parseServiceFullName(const std::string &full_name, std::string &service_name, std::string &method_name)
    {
        if (full_name.empty())
        {
            ERRORLOG("full name empty");
            return false;
        }
        size_t i = full_name.find_first_of(".");
        if (i == full_name.npos)
        {
            ERRORLOG("not find . in full name [%s]", full_name.c_str());
            return false;
        }
        service_name = full_name.substr(0, i);
        method_name = full_name.substr(i + 1);
        INFOLOG("parse service_name [%s] and method_name[%s] from full name [%s]", service_name.c_str(), method_name.c_str(), full_name.c_str());
        return true;
    }

    void RpcDispatcher::registerService(service_s_ptr service)
    {
        std::string service_name = service->GetDescriptor()->full_name();
        m_service_map[service_name] = service;
    }

    void RpcDispatcher::setTinyPBError(std::shared_ptr<TinyPBProtocol> msg, int32_t err_code, const std::string err_info)
    {
        msg->m_err_code = err_code;
        msg->m_err_info = err_info;
        msg->m_err_info_len = err_info.length();
    }
}