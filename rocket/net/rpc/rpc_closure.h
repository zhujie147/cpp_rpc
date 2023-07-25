#ifndef ROCKET_NET_RPC_RPC_CLOSURE_h
#define ROCKET_NET_RPC_RPC_CLOSURE_h

#include <google/protobuf/stubs/callback.h>
#include <functional>

namespace rocket
{

    class RpcClosure : public google::protobuf::Closure
    {
        public:
            void Run() override{
                if(m_cb!=nullptr){
                    m_cb();
                }
            }
        private:
            std::function<void()> m_cb {nullptr};

    };
}

#endif