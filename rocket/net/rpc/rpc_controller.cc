#include "rocket/net/rpc/rpc_controller.h"

namespace rocket {

// 重置 RPC 控制器的状态
void RpcController::Reset() {
  m_error_code = 0;          // 重新设置错误码为 0
  m_error_info = "";         // 清空错误信息
  m_msg_id = "";             // 清空消息 ID
  m_is_failed = false;       // 设置为未失败状态
  m_is_canceled = false;     // 设置为未取消状态
  m_local_addr = nullptr;    // 清空本地地址
  m_peer_addr = nullptr;     // 清空对等地址
  m_timeout = 1000;          // 设置默认超时时间为 1000 毫秒
}

// 检查 RPC 请求是否失败
bool RpcController::Failed() const {
  return m_is_failed;        // 返回是否失败的标志
}

// 获取错误信息
std::string RpcController::ErrorText() const {
  return m_error_info;       // 返回错误信息
}

// 开始取消 RPC 请求
void RpcController::StartCancel() {
  m_is_canceled = true;      // 设置为取消状态
}

// 设置失败的原因
void RpcController::SetFailed(const std::string& reason) {
  m_error_info = reason;     // 设置错误信息
}

// 检查 RPC 请求是否已被取消
bool RpcController::IsCanceled() const {
  return m_is_canceled;      // 返回是否已取消的标志
}

// 注册取消通知的回调函数（暂未实现）
void RpcController::NotifyOnCancel(google::protobuf::Closure* callback) {
  // 目前没有实现取消通知的回调逻辑
}

// 设置错误码和错误信息
void RpcController::SetError(int32_t error_code, const std::string& error_info) {
  m_error_code = error_code;        // 设置错误码
  m_error_info = error_info;        // 设置错误信息
}

// 获取错误码
int32_t RpcController::GetErrorCode() {
  return m_error_code;              // 返回错误码
}

// 获取错误信息
std::string RpcController::GetErrorInfo() {
  return m_error_info;              // 返回错误信息
}

// 设置消息 ID
void RpcController::SetMsgId(const std::string& msg_id) {
  m_msg_id = msg_id;                // 设置消息 ID
}

// 获取消息 ID
std::string RpcController::GetMsgId() {
  return m_msg_id;                  // 返回消息 ID
}

// 设置本地地址
void RpcController::SetLocalAddr(NetAddr::s_ptr addr) {
  m_local_addr = addr;             // 设置本地地址
}

// 设置对等地址
void RpcController::SetPeerAddr(NetAddr::s_ptr addr) {
  m_peer_addr = addr;              // 设置对等地址
}

// 获取本地地址
NetAddr::s_ptr RpcController::GetLocalAddr() {
  return m_local_addr;             // 返回本地地址
}

// 获取对等地址
NetAddr::s_ptr RpcController::GetPeerAddr() {
  return m_peer_addr;             // 返回对等地址
}

// 设置超时时间
void RpcController::SetTimeout(int timeout) {
  m_timeout = timeout;            // 设置超时时间
}

// 获取超时时间
int RpcController::GetTimeout() {
  return m_timeout;               // 返回超时时间
}

}  // namespace rocket
