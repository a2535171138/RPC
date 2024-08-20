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

namespace rocket {

// 构造函数，接受一个目标地址的智能指针，初始化 TCP 客户端
RpcChannel::RpcChannel(NetAddr::s_ptr peer_addr) : m_peer_addr(peer_addr) {
    m_client = make_shared<TcpClient>(m_peer_addr);
}

// 析构函数，释放资源
RpcChannel::~RpcChannel() {
    INFOLOG("~RpcChannel");
}

// 实现 RpcChannel 的 CallMethod 方法，用于发起 RPC 调用
void RpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                            google::protobuf::RpcController* controller,
                            const google::protobuf::Message* request,
                            google::protobuf::Message* response,
                            google::protobuf::Closure* done) {

    // 创建请求协议对象
    shared_ptr<rocket::TinyPBProtocol> req_protocol = make_shared<rocket::TinyPBProtocol>();

    // 将 controller 转换为 RpcController 类型
    RpcController* my_controller = dynamic_cast<RpcController*>(controller);
    if (my_controller == NULL) {
        ERRORLOG("failed call method, RpcController convert error");
        return;
    }

    // 生成或获取消息 ID
    if (my_controller->GetMsgId().empty()) {
        req_protocol->m_msg_id = MsgIDUtil::GenMsgID();
        my_controller->SetMsgId(req_protocol->m_msg_id);
    } else {
        req_protocol->m_msg_id = my_controller->GetMsgId();
    }

    // 设置方法名称
    req_protocol->m_method_name = method->full_name();
    INFOLOG("%s | call method name [%s]", req_protocol->m_msg_id.c_str(), req_protocol->m_method_name.c_str());

    // 检查是否已初始化
    if (!m_is_init) {
        string err_info = "RpcChannel not init";
        my_controller->SetError(ERROR_RPC_CHANNEL_INIT, err_info);
        ERRORLOG("%s | %s, RpcChannel not init", req_protocol->m_msg_id.c_str(), err_info.c_str());
        return;
    }

    // 序列化请求消息
    if (!request->SerializeToString(&(req_protocol->m_pb_data))) {
        string err_info = "failed to serialize";
        my_controller->SetError(ERROR_FAILED_SERIALIZE, err_info);
        ERRORLOG("%s | %s, origin request [%s]", req_protocol->m_msg_id.c_str(), err_info.c_str(), request->ShortDebugString().c_str());
        return;
    }

    // 创建当前对象的共享指针
    s_ptr channel = shared_from_this();

    // 连接服务器并发送请求
    m_client->connect([req_protocol, channel]() mutable {
        channel->getTcpClient()->writeMessage(req_protocol, [req_protocol, channel](AbstractProtocol::s_ptr) mutable {
            INFOLOG("%s | send rpc request success, call method name[%s]", req_protocol->m_msg_id.c_str(), req_protocol->m_method_name.c_str());

            // 读取服务器响应
            channel->getTcpClient()->readMessage(req_protocol->m_msg_id, [channel](AbstractProtocol::s_ptr msg) mutable {
                shared_ptr<rocket::TinyPBProtocol> rsp_protocol = dynamic_pointer_cast<rocket::TinyPBProtocol>(msg);
                INFOLOG("%s | success get rpc response, call method name[%s]", rsp_protocol->m_msg_id.c_str(), rsp_protocol->m_method_name.c_str());

                // 将响应消息反序列化
                RpcController* my_controller = dynamic_cast<RpcController*>(channel->getController());
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

                // 如果存在闭包回调，则执行
                if (channel->getClosure()) {
                    channel->getClosure()->Run();
                }

                // 释放当前对象的引用
                channel.reset();
            });
        });
    });
}

// 初始化 RpcChannel，包括控制器、请求、响应和回调
void RpcChannel::Init(controller_s_ptr controller, message_s_ptr req, message_s_ptr res, closure_s_ptr done) {
    if (m_is_init) {
        return;
    }

    m_controller = controller;
    m_request = req;
    m_response = res;
    m_closure = done;
    m_is_init = true;
}

// 获取控制器对象
google::protobuf::RpcController* RpcChannel::getController() {
    return m_controller.get();
}

// 获取请求消息对象
google::protobuf::Message* RpcChannel::getRequest() {
    return m_request.get();
}

// 获取响应消息对象
google::protobuf::Message* RpcChannel::getResponse() {
    return m_response.get();
}

// 获取回调闭包对象
google::protobuf::Closure* RpcChannel::getClosure() {
    return m_closure.get();
}

// 获取 TCP 客户端对象
TcpClient* RpcChannel::getTcpClient() {
    return m_client.get();
}

}
