#ifndef ROCKET_NET_RPC_RPC_CHANNEL_H
#define ROCKET_NET_RPC_RPC_CHANNEL_H

#include <google/protobuf/service.h>  // 引入 Google Protocol Buffers 的 RPC 服务头文件
#include <memory>  // 引入标准库的智能指针
#include "rocket/net/tcp/net_addr.h"  // 引入网络地址类
#include "rocket/net/tcp/tcp_client.h"  // 引入 TCP 客户端类
#include "rocket/net/timer_event.h"  // 引入定时事件类

namespace rocket {

// 定义了宏 NEWMESSAGE，用于简化消息对象的创建
#define NEWMESSAGE(type, var_name) \
  std::shared_ptr<type> var_name = std::make_shared<type>();  \

// 定义了宏 NEWRPCCONTROLLER，用于简化 RPC 控制器对象的创建
#define NEWRPCCONTROLLER(var_name) \
  std::shared_ptr<rocket::RpcController> var_name = std::make_shared<rocket::RpcController>();  \

// 定义了宏 NEWRPCCHANNEL，用于简化 RPC 通道对象的创建
#define NEWRPCCHANNEL(addr, var_name) \
  std::shared_ptr<rocket::RpcChannel> var_name = std::make_shared<rocket::RpcChannel>(std::make_shared<rocket::IPNetAddr>(addr));  \

// 定义了宏 CALLRPC，用于简化 RPC 调用的过程
#define CALLRPC(addr, stub_name, method_name, controller, request, response, closure) \
  { \
  NEWRPCCHANNEL(addr, channel); \
  channel->Init(controller, request, response, closure); \
  stub_name(channel.get()).method_name(controller.get(), request.get(), response.get(), closure.get()); \
  } \

// RpcChannel 类实现了 Google Protocol Buffers 的 RpcChannel 接口
class RpcChannel : public google::protobuf::RpcChannel, public enable_shared_from_this<RpcChannel> {
  public:
    // 定义智能指针类型，简化代码中使用智能指针的声明
    typedef std::shared_ptr<RpcChannel> s_ptr;
    typedef std::shared_ptr<google::protobuf::RpcController> controller_s_ptr;
    typedef std::shared_ptr<google::protobuf::Message> message_s_ptr;
    typedef std::shared_ptr<google::protobuf::Closure> closure_s_ptr;

    // 构造函数，接受一个远程地址作为参数，用于初始化 RpcChannel
    RpcChannel(NetAddr::s_ptr peer_addr);

    // 析构函数，负责清理资源
    ~RpcChannel();

    // 初始化函数，设置控制器、请求、响应和回调函数
    void Init(controller_s_ptr controller, message_s_ptr req, message_s_ptr res, closure_s_ptr done);

    // 重载的 CallMethod 函数，负责发起 RPC 调用
    void CallMethod(const google::protobuf::MethodDescriptor* method,
                    google::protobuf::RpcController* controller,
                    const google::protobuf::Message* request,
                    google::protobuf::Message* response,
                    google::protobuf::Closure* done);

    // 获取控制器对象
    google::protobuf::RpcController* getController();

    // 获取请求消息对象
    google::protobuf::Message* getRequest();

    // 获取响应消息对象
    google::protobuf::Message* getResponse();

    // 获取回调闭包对象
    google::protobuf::Closure* getClosure();

    // 获取 TCP 客户端对象
    TcpClient* getTcpClient();

    // 获取定时事件对象
    TimerEvent::s_ptr getTimerEvent();

  private:
    // 远程端的网络地址
    NetAddr::s_ptr m_peer_addr {nullptr};
    // 本地端的网络地址
    NetAddr::s_ptr m_local_addr {nullptr};

    // 控制器，用于管理 RPC 调用的状态
    controller_s_ptr m_controller {nullptr};
    // 请求消息
    message_s_ptr m_request {nullptr};
    // 响应消息
    message_s_ptr m_response {nullptr};
    // 回调闭包，当 RPC 调用完成后执行
    closure_s_ptr m_closure {nullptr};

    // 表示是否已初始化的标志
    bool m_is_init {false};

    // TCP 客户端，用于建立和管理与远程服务器的连接
    TcpClient::s_ptr m_client {nullptr};

    // 定时事件，用于处理超时等情况
    TimerEvent::s_ptr m_timer_event {nullptr};
};

}  // namespace rocket

#endif  // ROCKET_NET_RPC_RPC_CHANNEL_H
