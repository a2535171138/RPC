#ifndef ROCKET_NET_RPC_RPC_CONTROLLER_H
#define ROCKET_NET_RPC_RPC_CONTROLLER_H

#include <google/protobuf/service.h>  // Google Protocol Buffers的RpcController基类
#include <google/protobuf/stubs/callback.h>  // Google Protocol Buffers的回调函数头文件
#include <string>  // 标准字符串库
#include "rocket/net/tcp/net_addr.h"  // 网络地址类定义

namespace rocket {

// RpcController 类继承自 google::protobuf::RpcController，用于控制RPC请求的执行状态
class RpcController : public google::protobuf::RpcController {
 public:
  RpcController() {}
  ~RpcController() {}

  // 重置控制器的状态
  void Reset();

  // 检查RPC请求是否失败
  bool Failed() const;

  // 获取失败的错误信息
  std::string ErrorText() const;

  // 开始取消RPC请求
  void StartCancel();

  // 设置失败的原因
  void SetFailed(const std::string& reason);

  // 检查RPC请求是否已被取消
  bool IsCanceled() const;

  // 注册取消通知的回调函数
  void NotifyOnCancel(google::protobuf::Closure* callback);

  // 设置错误码和错误信息
  void SetError(int32_t error_code, const std::string& error_info);

  // 获取错误码
  int32_t GetErrorCode();

  // 获取错误信息
  std::string GetErrorInfo();

  // 设置消息ID
  void SetMsgId(const std::string& msg_id);

  // 获取消息ID
  std::string GetMsgId();

  // 设置本地地址
  void SetLocalAddr(NetAddr::s_ptr addr);

  // 设置对等地址（即远程地址）
  void SetPeerAddr(NetAddr::s_ptr addr);

  // 获取本地地址
  NetAddr::s_ptr GetLocalAddr();

  // 获取对等地址
  NetAddr::s_ptr GetPeerAddr();

  // 设置超时时间
  void SetTimeout(int timeout);

  // 获取超时时间
  int GetTimeout();

 private:
  // 错误码
  int32_t m_error_code {0};

  // 错误信息
  std::string m_error_info;

  // 消息ID
  std::string m_msg_id;

  // 是否失败
  bool m_is_failed {false};

  // 是否已取消
  bool m_is_canceled {false};

  // 本地地址
  NetAddr::s_ptr m_local_addr;

  // 对等地址
  NetAddr::s_ptr m_peer_addr;

  // 超时时间，单位为毫秒
  int m_timeout {1000};
};

}  // namespace rocket

#endif  // ROCKET_NET_RPC_RPC_CONTROLLER_H
