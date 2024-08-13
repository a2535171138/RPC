#ifndef ROCKET_NET_RPC_RPC_DISPATCHER_H
#define ROCKET_NET_RPC_RPC_DISPATCHER_H

#include <memory>
#include <google/protobuf/service.h>
#include "rocket/net/coder/abstract_protocol.h"
#include "rocket/net/coder/tinypb_protocol.h"

using namespace std;

namespace rocket {

// 前向声明 TcpConnection 类
class TcpConnection;

// RpcDispatcher 类用于处理 RPC 请求的调度
class RpcDispatcher {
  public:
    // 获取 RpcDispatcher 单例实例
    static RpcDispatcher* GetRpcDispatcher();

    // 定义一个服务指针类型
    typedef shared_ptr<google::protobuf::Service> service_s_ptr;
    
    // 调度请求，将请求分发给对应的服务
    void dispatch(AbstractProtocol::s_ptr request, AbstractProtocol::s_ptr response, TcpConnection* connection);

    // 注册服务到调度器
    void registerService(service_s_ptr service);

    // 设置 TinyPB 错误信息
    void setTinyPBError(shared_ptr<TinyPBProtocol> msg, int32_t err_code, const string& err_info);

  private:
    // 解析服务的全名，提取服务名和方法名
    bool parseServiceFullName(const string& full_name, string& service_name, string& method_name);

    // 存储服务映射（服务名到服务指针的映射）
    map<string, service_s_ptr> m_service_map;
};

}

#endif
