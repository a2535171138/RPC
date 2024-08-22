#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include "rocket/net/rpc/rpc_channel.h"
#include "rocket/net/rpc/rpc_controller.h"
#include "rocket/net/coder/tinypb_protocol.h"
#include "rocket/net/tcp/tcp_client.h"
#include "rocket/common/log.h"
#include "rocket/common/msg_id_util.h"
#include "rocket/common/error_code.h"
#include "rocket/net/timer_event.h"

namespace rocket {

// RpcChannel 构造函数，接受一个目标地址的智能指针，初始化一个 TCP 客户端以便与该地址进行通信
RpcChannel::RpcChannel(NetAddr::s_ptr peer_addr) : m_peer_addr(peer_addr) {
    m_client = make_shared<TcpClient>(m_peer_addr);
}

// RpcChannel 析构函数，释放资源并记录销毁日志
RpcChannel::~RpcChannel() {
    INFOLOG("~RpcChannel");
}

// 实现 RpcChannel 的 CallMethod 方法，用于发起 RPC 调用
void RpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                            google::protobuf::RpcController* controller,
                            const google::protobuf::Message* request,
                            google::protobuf::Message* response,
                            google::protobuf::Closure* done) {

    // 创建用于请求的协议对象 TinyPBProtocol
    shared_ptr<rocket::TinyPBProtocol> req_protocol = make_shared<rocket::TinyPBProtocol>();

    // 将 controller 转换为自定义的 RpcController 类型
    RpcController* my_controller = dynamic_cast<RpcController*>(controller);
    if (my_controller == NULL) {
        ERRORLOG("failed call method, RpcController convert error");
        return;
    }

    // 生成或获取消息 ID，用于唯一标识本次 RPC 调用
    if (my_controller->GetMsgId().empty()) {
        req_protocol->m_msg_id = MsgIDUtil::GenMsgID();
        my_controller->SetMsgId(req_protocol->m_msg_id);
    } else {
        req_protocol->m_msg_id = my_controller->GetMsgId();
    }

    // 设置调用的方法名称
    req_protocol->m_method_name = method->full_name();
    INFOLOG("%s | call method name [%s]", req_protocol->m_msg_id.c_str(), req_protocol->m_method_name.c_str());

    // 检查 RpcChannel 是否已经初始化
    if (!m_is_init) {
        string err_info = "RpcChannel not init";
        my_controller->SetError(ERROR_RPC_CHANNEL_INIT, err_info);
        ERRORLOG("%s | %s, RpcChannel not init", req_protocol->m_msg_id.c_str(), err_info.c_str());
        return;
    }

    // 序列化请求消息，将请求对象转换为字符串形式存储在协议对象中
    if (!request->SerializeToString(&(req_protocol->m_pb_data))) {
        string err_info = "failed to serialize";
        my_controller->SetError(ERROR_FAILED_SERIALIZE, err_info);
        ERRORLOG("%s | %s, origin request [%s]", req_protocol->m_msg_id.c_str(), err_info.c_str(), request->ShortDebugString().c_str());
        return;
    }

    // 创建当前 RpcChannel 对象的共享指针，用于保持当前对象的生命周期
    s_ptr channel = shared_from_this();

    // 设置定时事件，超时时执行取消操作并触发错误处理
    m_timer_event = make_shared<TimerEvent>(my_controller->GetTimeout(), false, [my_controller, channel]() mutable {
        my_controller->StartCancel(); // 启动取消操作
        my_controller->SetError(ERROR_RPC_CALL_TIMEOUT, "rpc call timeout " + to_string(my_controller->GetTimeout()));

        // 如果设置了回调函数，则在超时后执行
        if(channel->getClosure()){
            channel->getClosure()->Run();
        }

        channel.reset(); // 释放共享指针，结束当前对象的生命周期
    });

    m_client->addTimerEvent(m_timer_event);

    // 连接服务器并发送请求消息
    m_client->connect([req_protocol, channel]() mutable {

        RpcController* my_controller = dynamic_cast<RpcController*>(channel->getController());

        // 如果连接服务器出错，记录错误信息并返回
        if(channel->getTcpClient()->getConnectErrorCode() != 0){
            my_controller->SetError(channel->getTcpClient()->getConnectErrorCode(), channel->getTcpClient()->getConnectErrorInfo());
            ERRORLOG("%s | connect error, error code[%d], error info[%s], peer addr[%s]",
            req_protocol->m_msg_id.c_str(), my_controller->GetErrorCode(),
            my_controller->GetErrorInfo().c_str(), channel->getTcpClient()->getPeerAddr()->toString().c_str());
            return;
        }

        // 连接成功后，发送请求消息
        channel->getTcpClient()->writeMessage(req_protocol, [req_protocol, channel, my_controller](AbstractProtocol::s_ptr) mutable {
            INFOLOG("%s | send rpc request success, call method name[%s], peer addr[%s], local addr[%s]", 
            req_protocol->m_msg_id.c_str(), req_protocol->m_method_name.c_str(),
            channel->getTcpClient()->getPeerAddr()->toString().c_str(), channel->getTcpClient()->getLocalAddr()->toString().c_str());

            // 发送成功后，从服务器读取响应消息
            channel->getTcpClient()->readMessage(req_protocol->m_msg_id, [channel, my_controller](AbstractProtocol::s_ptr msg) mutable {
                shared_ptr<rocket::TinyPBProtocol> rsp_protocol = dynamic_pointer_cast<rocket::TinyPBProtocol>(msg);
                INFOLOG("%s | success get rpc response, call method name[%s], peer addr[%s], local addr[%s]", 
                rsp_protocol->m_msg_id.c_str(), rsp_protocol->m_method_name.c_str(),
                channel->getTcpClient()->getPeerAddr()->toString().c_str(), channel->getTcpClient()->getLocalAddr()->toString().c_str());
                
                // 成功获取响应后，取消定时任务
                channel->getTimerEvent()->setCanceled(true);

                // 将响应消息反序列化，将其转换为对象
                if (!(channel->getResponse()->ParseFromString(rsp_protocol->m_pb_data))) {
                    ERRORLOG("%s | serialize error", rsp_protocol->m_msg_id.c_str());
                    my_controller->SetError(ERROR_FAILED_SERIALIZE, "serialize error");
                    return;
                }

                // 检查服务器返回的错误码
                if (rsp_protocol->m_err_code != 0) {
                    ERRORLOG("%s | call rpc method[%s] failed, error code[%d], error info[%s]",
                    rsp_protocol->m_msg_id.c_str(), rsp_protocol->m_method_name.c_str(),
                    rsp_protocol->m_err_code, rsp_protocol->m_err_info.c_str());

                    my_controller->SetError(rsp_protocol->m_err_code, rsp_protocol->m_err_info);
                    return;
                }

                INFOLOG("%s | call rpc success, call method name[%s], peer addr[%s], local addr[%s]", 
                rsp_protocol->m_msg_id.c_str(), rsp_protocol->m_method_name.c_str(),
                channel->getTcpClient()->getPeerAddr()->toString().c_str(), channel->getTcpClient()->getLocalAddr()->toString().c_str());

                // 如果调用未被取消，并且设置了回调函数，则执行回调
                if (!my_controller->IsCanceled() && channel->getClosure()) {
                    channel->getClosure()->Run();
                }

                // 释放当前对象的引用，结束其生命周期
                channel.reset();
            });
        });
    });
}

// 初始化 RpcChannel，包括设置控制器、请求消息、响应消息和回调函数
void RpcChannel::Init(controller_s_ptr controller, message_s_ptr req, message_s_ptr res, closure_s_ptr done) {
    if (m_is_init) {
        return; // 如果已经初始化，则不再重复初始化
    }

    m_controller = controller;
    m_request = req;
    m_response = res;
    m_closure = done;
    m_is_init = true; // 标记为已初始化
}

// 获取控制器对象的指针
google::protobuf::RpcController* RpcChannel::getController() {
    return m_controller.get();
}

// 获取请求消息对象的指针
google::protobuf::Message* RpcChannel::getRequest() {
    return m_request.get();
}

// 获取响应消息对象的指针
google::protobuf::Message* RpcChannel::getResponse() {
    return m_response.get();
}

// 获取回调闭包对象的指针
google::protobuf::Closure* RpcChannel::getClosure() {
    return m_closure.get();
}

// 获取 TCP 客户端对象的指针
TcpClient* RpcChannel::getTcpClient() {
    return m_client.get();
}

// 获取定时事件对象的智能指针
TimerEvent::s_ptr RpcChannel::getTimerEvent(){
    return m_timer_event;
}

}
