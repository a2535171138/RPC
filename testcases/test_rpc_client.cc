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

  // 创建一个网络地址对象，指定IP地址为127.0.0.1，端口号为12345
  rocket::IPNetAddr::s_ptr addr = std::make_shared<rocket::IPNetAddr>("127.0.0.1", 12345);

  // 创建TCP客户端对象，并传入前面创建的网络地址对象
  rocket::TcpClient client(addr);

  // 连接到指定地址，并在连接成功后执行传入的回调函数
  client.connect([addr, &client]() {
    // 连接成功后，记录成功日志
    DEBUGLOG("connect to [%s] success", addr->toString().c_str());

    // 创建一个TinyPB协议的消息对象，用于发送消息
    std::shared_ptr<rocket::TinyPBProtocol> message = std::make_shared<rocket::TinyPBProtocol>();
    message->m_msg_id = "99998888";  // 设置消息ID，唯一标识一条消息
    message->m_pb_data = "test pb data";  // 设置消息的初始数据

    // 创建一个makeOrderRequest对象，并设置订单价格和商品信息
    makeOrderRequest request;
    request.set_price(100);  // 设置订单价格
    request.set_goods("apple");  // 设置商品名称

    // 将请求对象序列化为字符串，并存入消息对象的pb_data字段中
    if (!request.SerializeToString(&(message->m_pb_data))) {
      // 如果序列化失败，记录错误日志并返回
      ERRORLOG("serialize error");
      return;
    }

    // 设置消息的方法名称，即将调用的远程方法的名称
    message->m_method_name = "Order.makeOrder";

    // 发送消息，并在消息发送成功后执行传入的回调函数
    client.writeMessage(message, [request](rocket::AbstractProtocol::s_ptr msg_ptr) {
      // 记录发送消息成功的日志，包括请求的详细信息
      DEBUGLOG("send message success, request[%s]", request.ShortDebugString().c_str());
    });

    // 读取消息，并在消息读取成功后执行传入的回调函数
    client.readMessage("99998888", [](rocket::AbstractProtocol::s_ptr msg_ptr) {
      // 将读取到的消息转换为TinyPB协议消息类型
      std::shared_ptr<rocket::TinyPBProtocol> message = std::dynamic_pointer_cast<rocket::TinyPBProtocol>(msg_ptr);
      // 记录收到的消息日志，包括消息ID和内容
      DEBUGLOG("msg_id[%s], get response %s", message->m_msg_id.c_str(), message->m_pb_data.c_str());

      // 创建makeOrderResponse对象，并尝试解析消息数据
      makeOrderResponse response;
      if (!response.ParseFromString(message->m_pb_data)) {
        // 如果解析失败，记录错误日志并返回
        ERRORLOG("deserialize error");
        return;
      }
      // 记录解析成功的响应日志，包括响应的详细信息
      DEBUGLOG("get response success, response[%s]", response.ShortDebugString().c_str());
    });
  });
}

// 测试RPC通道的函数
void test_rpc_channel() {
  // 使用宏定义创建一个新的RPC通道对象，连接到指定的服务地址127.0.0.1:12355
  NEWRPCCHANNEL("127.0.0.1:12355", channel);

  // 使用宏定义创建新的RPC请求对象和响应对象
  NEWMESSAGE(makeOrderRequest, request);
  NEWMESSAGE(makeOrderResponse, response);

  // 设置请求对象的具体参数
  request->set_price(100);  // 设置订单价格
  request->set_goods("apple");  // 设置商品名称

  // 使用宏定义创建新的RPC控制器对象，并设置消息ID和超时时间
  NEWRPCCONTROLLER(controller);
  controller->SetMsgId("99998888");  // 设置消息ID
  controller->SetTimeout(10000);  // 设置超时时间为10秒

  // 创建RPC闭包对象，并定义在RPC调用完成后要执行的操作
  shared_ptr<rocket::RpcClosure> closure = make_shared<rocket::RpcClosure>([request, response, channel, controller]() mutable {
    // 判断RPC调用是否成功
    if(controller->GetErrorCode() == 0){
      // 调用成功，记录成功日志，包括请求和响应的详细信息
      INFOLOG("call rpc success, request [%s], response [%s]", request->ShortDebugString().c_str(), response->ShortDebugString().c_str());

      // 执行业务逻辑，示例中用伪代码表示
      if(response->order_id() == "xxx"){
        // xxx
      }
    }
    else {
      // 调用失败，记录错误日志，包括错误代码和错误信息
      ERRORLOG("call rpc failed, request[%s], error code[%d], error info[%s]",
      request->ShortDebugString().c_str(),
      controller->GetErrorCode(),
      controller->GetErrorInfo().c_str());

    }

    // 记录退出事件循环的日志
    INFOLOG("now exit eventloop");
    channel->getTcpClient()->stop();  // 停止TCP客户端
    channel.reset();  // 释放RPC通道对象
  });

  // 发送RPC请求，并指定服务地址、方法、请求对象、响应对象和回调闭包
  CALLRPC("127.0.0.1:12355", Order_Stub, makeOrder, controller, request, response, closure);
}

// 主函数，程序入口
int main() {
  // 设置全局配置文件，指定配置文件路径为../conf/rocket.xml
  rocket::Config::SetGlobalConfig("../conf/rocket.xml");

  // 初始化全局日志记录器
  rocket::Logger::InitGlobalLogger();

  // 执行TCP客户端测试函数（当前被注释掉，未执行）
  // test_tcp_client();
  
  // 执行RPC通道测试函数
  test_rpc_channel();

  return 0;  // 正常结束程序
}
