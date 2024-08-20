#include <assert.h>  // 断言库，用于程序调试和验证条件
#include <sys/socket.h>  // 套接字库，提供网络通信功能
#include <fcntl.h>  // 文件控制库，用于文件操作的控制
#include <string.h>  // 字符串处理库
#include <pthread.h>  // 线程库，提供线程操作功能
#include <arpa/inet.h>  // 地址族和协议库，用于网络地址处理
#include <netinet/in.h>  // 网络协议库，提供网络协议的常量和结构
#include <string>  // 标准字符串库
#include <memory>  // 智能指针库，用于管理动态分配的对象
#include <unistd.h>  // 提供对POSIX操作系统API的访问
#include <google/protobuf/service.h>  // Google Protocol Buffers服务库
#include "rocket/common/log.h"  // 自定义日志头文件
#include "rocket/common/config.h"  // 自定义配置头文件
#include "rocket/net/tcp/tcp_client.h"  // 自定义TCP客户端头文件
#include "rocket/net/tcp/net_addr.h"  // 自定义网络地址头文件
#include "rocket/net/coder/string_coder.h"  // 自定义字符串编码器头文件
#include "rocket/net/coder/abstract_protocol.h"  // 自定义抽象协议头文件
#include "rocket/net/coder/tinypb_coder.h"  // 自定义TinyPB编码器头文件
#include "rocket/net/coder/tinypb_protocol.h"  // 自定义TinyPB协议头文件
#include "rocket/net/tcp/tcp_server.h"  // 自定义TCP服务器头文件
#include "rocket/net/rpc/rpc_dispatcher.h"  // 自定义RPC分发器头文件
#include "order.pb.h"  // 生成的Protocol Buffers消息头文件

// OrderImpl 类继承自 Order，用于实现具体的 RPC 服务
class OrderImpl : public Order {
 public:
  // 实现RPC服务的方法
  void makeOrder(google::protobuf::RpcController* controller,
                      const ::makeOrderRequest* request,
                      ::makeOrderResponse* response,
                      ::google::protobuf::Closure* done) {

    // 检查订单价格，如果价格小于10，则返回错误信息
    if (request->price() < 10) {
      response->set_ret_code(-1);  // 设置返回码为-1，表示错误
      response->set_res_info("short balance");  // 设置返回信息，表示余额不足
      return;
    }
    // 设置订单ID
    response->set_order_id("20240514");
  }

};

// 测试TCP服务器的函数
void test_tcp_server() {
  // 创建网络地址对象，IP地址为127.0.0.1，端口号为12345
  rocket::IPNetAddr::s_ptr addr = std::make_shared<rocket::IPNetAddr>("127.0.0.1", 12350);

  // 记录创建的地址信息
  DEBUGLOG("create addr %s", addr->toString().c_str());

  // 创建TCP服务器对象，并传入网络地址
  rocket::TcpServer tcp_server(addr);

  // 启动TCP服务器
  tcp_server.start();
}

// 主函数
int main(int argc, char* argv[]) {

  // 设置全局配置，配置文件路径为../conf/rocket.xml
  rocket::Config::SetGlobalConfig("../conf/rocket.xml");

  // 初始化全局日志记录器
  rocket::Logger::InitGlobalLogger();

  // 创建OrderImpl服务对象，并注册到RPC分发器
  std::shared_ptr<OrderImpl> service = std::make_shared<OrderImpl>();
  rocket::RpcDispatcher::GetRpcDispatcher()->registerService(service);

  // 执行TCP服务器测试函数
  test_tcp_server();

  return 0;  // 正常结束程序
}
