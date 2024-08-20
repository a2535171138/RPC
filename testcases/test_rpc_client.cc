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
#include "rocket/net/rpc/rpc_controller.h"  // 自定义RPC控制器头文件
#include "rocket/net/rpc/rpc_channel.h"  // 自定义RPC通道头文件
#include "rocket/net/rpc/rpc_closure.h"  // 自定义RPC闭包头文件

#include "order.pb.h"  // 生成的Protocol Buffers消息头文件

// 测试TCP客户端的函数
void test_tcp_client() {

  // 创建网络地址对象，IP地址为127.0.0.1，端口号为12345
  rocket::IPNetAddr::s_ptr addr = std::make_shared<rocket::IPNetAddr>("127.0.0.1", 12345);

  // 创建TCP客户端对象，并传入网络地址
  rocket::TcpClient client(addr);

  // 连接到指定地址，并在连接成功后执行回调函数
  client.connect([addr, &client]() {
    // 连接成功后，记录日志
    DEBUGLOG("connect to [%s] success", addr->toString().c_str());

    // 创建TinyPB协议的消息对象
    std::shared_ptr<rocket::TinyPBProtocol> message = std::make_shared<rocket::TinyPBProtocol>();
    message->m_msg_id = "99998888";  // 设置消息ID
    message->m_pb_data = "test pb data";  // 设置初始的消息数据

    // 创建一个makeOrderRequest对象，并设置订单价格和商品
    makeOrderRequest request;
    request.set_price(100);
    request.set_goods("apple");

    // 将请求对象序列化为字符串并存入消息的pb_data字段中
    if (!request.SerializeToString(&(message->m_pb_data))) {
      // 如果序列化失败，记录错误日志并返回
      ERRORLOG("serialize error");
      return;
    }

    // 设置消息的方法名称
    message->m_method_name = "Order.makeOrder";

    // 发送消息，并在消息发送成功后执行回调函数
    client.writeMessage(message, [request](rocket::AbstractProtocol::s_ptr msg_ptr) {
      // 记录发送消息成功的日志
      DEBUGLOG("send message success, request[%s]", request.ShortDebugString().c_str());
    });

    // 读取消息，并在消息读取成功后执行回调函数
    client.readMessage("99998888", [](rocket::AbstractProtocol::s_ptr msg_ptr) {
      // 将读取到的消息转换为TinyPB协议消息
      std::shared_ptr<rocket::TinyPBProtocol> message = std::dynamic_pointer_cast<rocket::TinyPBProtocol>(msg_ptr);
      // 记录收到的消息日志
      DEBUGLOG("msg_id[%s], get response %s", message->m_msg_id.c_str(), message->m_pb_data.c_str());

      // 创建makeOrderResponse对象，并解析消息数据
      makeOrderResponse response;
      if (!response.ParseFromString(message->m_pb_data)) {
        // 如果解析失败，记录错误日志并返回
        ERRORLOG("deserialize error");
        return;
      }
      // 记录解析成功的响应日志
      DEBUGLOG("get response success, response[%s]", response.ShortDebugString().c_str());
    });
  });
}

// 测试RPC通道的函数
void test_rpc_channel() {
  // 创建网络地址对象，IP地址为127.0.0.1，端口号为12350
  rocket::IPNetAddr::s_ptr addr = make_shared<rocket::IPNetAddr>("127.0.0.1", 12350);
  // 创建RPC通道对象，并传入网络地址
  shared_ptr<rocket::RpcChannel> channel = make_shared<rocket::RpcChannel>(addr);

  // 创建请求和响应对象
  shared_ptr<makeOrderRequest> request = make_shared<makeOrderRequest>();
  request->set_price(100);
  request->set_goods("apple");

  shared_ptr<makeOrderResponse> response = make_shared<makeOrderResponse>();

  // 创建RPC控制器对象，并设置消息ID
  shared_ptr<rocket::RpcController> controller = make_shared<rocket::RpcController>();
  controller->SetMsgId("99998888");

  // 创建RPC闭包对象，并定义完成后要执行的操作
  shared_ptr<rocket::RpcClosure> closure = make_shared<rocket::RpcClosure>([request, response, channel]() mutable {
    INFOLOG("call rpc success, request[%s], response[%s]", request->ShortDebugString().c_str(), response->ShortDebugString().c_str());
    INFOLOG("now exit eventloop");
    channel->getTcpClient()->stop();  // 停止TCP客户端
    channel.reset();  // 释放RPC通道对象
  });

  // 初始化RPC通道，传入控制器、请求、响应和闭包
  channel->Init(controller, request, response, closure);

  // 创建Order服务的Stub对象，并发起RPC调用
  Order_Stub stub(channel.get());

  stub.makeOrder(controller.get(), request.get(), response.get(), closure.get());  // 发起RPC调用
}

// 主函数
int main() {
  // 设置全局配置，配置文件路径为../conf/rocket.xml
  rocket::Config::SetGlobalConfig("../conf/rocket.xml");

  // 初始化全局日志记录器
  rocket::Logger::InitGlobalLogger();

  // 执行TCP客户端测试函数
  // test_tcp_client();
  
  // 执行RPC通道测试函数
  test_rpc_channel();

  return 0;  // 正常结束程序
}
