#ifndef ROCKET_NET_RPC_RPC_CLOSURE_H
#define ROCKET_NET_RPC_RPC_CLOSURE_H

#include <google/protobuf/stubs/callback.h>  // Google Protocol Buffers的回调函数头文件
#include <functional>  // 标准函数库，用于定义回调函数

namespace rocket {

// RpcClosure 类继承自 google::protobuf::Closure，用于封装回调函数
class RpcClosure : public google::protobuf::Closure {
 public:
  // 重写父类的 Run 方法，用于执行回调函数
  void Run() override {
    // 如果回调函数存在，则调用它
    if (m_cb != nullptr) {
      m_cb();
    }
  }

 private:
  // 回调函数对象，初始化为nullptr
  std::function<void()> m_cb {nullptr};
};

}  // namespace rocket

#endif  // ROCKET_NET_RPC_RPC_CLOSURE_H
