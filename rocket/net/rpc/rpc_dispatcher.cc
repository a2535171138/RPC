#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include "rocket/net/rpc/rpc_dispatcher.h"
#include "rocket/net/coder/tinypb_protocol.h"
#include "rocket/common/log.h"
#include "rocket/common/error_code.h"
#include "rocket/net/rpc/rpc_controller.h"
#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/tcp/tcp_connection.h"

namespace rocket{

// 静态指针，指向 RpcDispatcher 的单例实例
static RpcDispatcher* g_rpc_dispatcher = NULL;

// 获取 RpcDispatcher 的单例实例
RpcDispatcher* RpcDispatcher::GetRpcDispatcher(){
  if(g_rpc_dispatcher != NULL){
    return g_rpc_dispatcher; // 如果实例已存在，直接返回
  }
  g_rpc_dispatcher = new RpcDispatcher; // 创建新的实例
  return g_rpc_dispatcher;
}

// 调度 RPC 请求，将请求分发到注册的服务
void RpcDispatcher::dispatch(AbstractProtocol::s_ptr request, AbstractProtocol::s_ptr response, TcpConnection* connection){
  // 将请求和响应协议转换为 TinyPBProtocol 类型
  shared_ptr<TinyPBProtocol> req_protocol = dynamic_pointer_cast<TinyPBProtocol>(request);
  shared_ptr<TinyPBProtocol> rsp_protocol = dynamic_pointer_cast<TinyPBProtocol>(response);

  // 获取方法的完整名称
  string method_full_name = req_protocol->m_method_name;
  string service_name;
  string method_name;

  // 设置响应协议的消息 ID 和方法名称
  rsp_protocol->m_msg_id = req_protocol->m_msg_id;
  rsp_protocol->m_method_name = req_protocol->m_method_name;

  // 解析服务的全名，提取服务名和方法名
  if(!parseServiceFullName(method_full_name, service_name, method_name)){
    setTinyPBError(rsp_protocol, ERROR_PARSE_SERVICE_NAME, "parse service name error");
    return;
  }

  // 查找服务
  auto it = m_service_map.find(service_name);
  if(it == m_service_map.end()){
    ERRORLOG("%s | service name[%s] not found", req_protocol->m_msg_id.c_str(), service_name.c_str());
    setTinyPBError(rsp_protocol, ERROR_SERVICE_NOT_FOUND, "service not found");
    return;
  }

  // 获取服务对象
  service_s_ptr service = (*it).second;

  // 查找方法描述符
  const google::protobuf::MethodDescriptor* method = service->GetDescriptor()->FindMethodByName(method_name);
  if(method == NULL){
    ERRORLOG("%s | method name[%s] not found in service[%s]", req_protocol->m_msg_id.c_str(), method_name.c_str(), service_name.c_str());
    setTinyPBError(rsp_protocol, ERROR_SERVICE_NOT_FOUND, "method not found");
    return;
  }

  // 创建请求消息对象
  google::protobuf::Message* req_msg = service->GetRequestPrototype(method).New();

  // 解析请求数据
  if(!req_msg->ParseFromString(req_protocol->m_pb_data)){
    ERRORLOG("%s | deserialize error", req_protocol->m_msg_id.c_str(), method_name.c_str(), service_name.c_str());
    setTinyPBError(rsp_protocol, ERROR_FAILED_DESERIALIZE, "deserialize error");
    delete req_msg;
    return;
  }

  INFOLOG("%s | get rpc request[%s]", req_protocol->m_msg_id.c_str(), req_msg->ShortDebugString().c_str());

  // 创建响应消息对象
  google::protobuf::Message* rsp_msg = service->GetResponsePrototype(method).New();

  // 创建 RPC 控制器
  RpcController* rpc_controller = new RpcController();
  rpc_controller->SetLocalAddr(connection->getLocalAddr());
  rpc_controller->SetPeerAddr(connection->getPeerAddr());
  rpc_controller->SetMsgId(req_protocol->m_msg_id);

  // 调用服务方法
  service->CallMethod(method, rpc_controller, req_msg, rsp_msg, nullptr);

  // 序列化响应数据
  if(!rsp_msg->SerializeToString(&(rsp_protocol->m_pb_data))){
    ERRORLOG("%s | serialize error, original message [%s]", req_protocol->m_msg_id.c_str(), rsp_msg->ShortDebugString().c_str());
    setTinyPBError(rsp_protocol, ERROR_SERVICE_NOT_FOUND, "serialize error");
    delete req_msg;
    delete rsp_msg;
    return;
  }

  // 设置响应协议的错误码为 0，表示成功
  rsp_protocol->m_err_code = 0;

  INFOLOG("%s | dispatch success, request[%s], response[%s]", req_protocol->m_msg_id.c_str(), req_msg->ShortDebugString().c_str(), rsp_msg->ShortDebugString().c_str());

  // 释放内存
  delete req_msg;
  delete rsp_msg;
}

// 解析服务的全名，提取服务名和方法名
bool RpcDispatcher::parseServiceFullName(const string& full_name, string& service_name, string& method_name){
  if(full_name.empty()){
    ERRORLOG("full name empty");
    return false;
  }
  size_t i = full_name.find_first_of(".");
  if(i == full_name.npos){
    ERRORLOG("not find . in full name [%s]", full_name.c_str());
    return false;
  }

  // 提取服务名和方法名
  service_name = full_name.substr(0, i);
  method_name = full_name.substr(i + 1, full_name.length() - i - 1);

  INFOLOG("parse service_name[%s] and method_name[%s] from full name [%s]", service_name.c_str(), method_name.c_str(), full_name.c_str());

  return true;
}

// 注册服务到调度器
void RpcDispatcher::registerService(service_s_ptr service){
  string service_name = service->GetDescriptor()->full_name();
  m_service_map[service_name] = service;
}

// 设置 TinyPB 错误信息
void RpcDispatcher::setTinyPBError(shared_ptr<TinyPBProtocol> msg, int32_t err_code, const string& err_info){
  msg->m_err_code = err_code;
  msg->m_err_info = err_info;
  msg->m_err_info_len = err_info.length();
}

}
