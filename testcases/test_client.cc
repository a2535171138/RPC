#include <assert.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <memory>
#include <unistd.h>
#include "rocket/common/log.h"
#include "rocket/common/config.h"
#include "rocket/net/tcp/tcp_client.h"
#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/coder/string_coder.h"
#include "rocket/net/coder/abstract_protocol.h"
#include "rocket/net/coder/tinypb_coder.h"
#include "rocket/net/coder/tinypb_protocol.h"

// 测试连接函数
void test_connect() {
  // 创建一个 TCP 套接字
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  
  // 检查套接字是否创建成功
  if (fd < 0) {
    ERRORLOG("invalid fd %d", fd);  // 记录错误日志
    exit(0);  // 退出程序
  }

  // 设置服务器地址
  sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));  // 清零结构体
  server_addr.sin_family = AF_INET;  // 地址族
  server_addr.sin_port = htons(12351);  // 端口号，使用网络字节顺序
  inet_aton("127.0.0.1", &server_addr.sin_addr);  // 设置 IP 地址为本地地址

  // 连接到服务器
  int rt = connect(fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));

  // 检查连接是否成功
  if (rt < 0) {
    ERRORLOG("connect failed, fd[%d]", fd);  // 记录错误日志
    close(fd);  // 关闭套接字
    return;
  }

  // 发送数据到服务器
  string msg = "hello rocket!";
  rt = write(fd, msg.c_str(), msg.length());  // 写入数据
  DEBUGLOG("success write %d bytes, [%s]", rt, msg.c_str());  // 记录调试日志

  // 从服务器读取数据
  char buf[100];  // 数据缓冲区
  rt = read(fd, buf, 100);  // 读取数据
  if (rt < 0) {
    ERRORLOG("read failed, fd[%d]", fd);  // 记录错误日志
  } else {
    DEBUGLOG("success read %d bytes, [%s]", rt, string(buf).c_str());  // 记录调试日志
  }

  // 关闭套接字
  close(fd);
}

// 测试 TcpClient 类
void test_tcp_client() {
  rocket::IPNetAddr::s_ptr addr = make_shared<rocket::IPNetAddr>("127.0.0.1", 12350);
  rocket::TcpClient client(addr);
  
  // 连接到服务器
  client.connect([addr, &client]() {
    DEBUGLOG("connect to [%s] success", addr->toString().c_str());
    
    // 创建并发送消息
    shared_ptr<rocket::TinyPBProtocol> message = make_shared<rocket::TinyPBProtocol>();
    message->m_msg_id = "123456789";
    message->m_pb_data = "test pb data";
    
    client.writeMessage(message, [](rocket::AbstractProtocol::s_ptr msg_ptr) {
      DEBUGLOG("send message success");
    });

    // 读取消息
    client.readMessage("123456789", [](rocket::AbstractProtocol::s_ptr msg_ptr) {
      shared_ptr<rocket::TinyPBProtocol> message = dynamic_pointer_cast<rocket::TinyPBProtocol>(msg_ptr);
      DEBUGLOG("msg_id[%s], get response %s", message->m_msg_id.c_str(), message->m_pb_data.c_str());
    });

  });
}

int main() {
  // 禁用 stdout 的缓冲区，这样 printf 直接输出到终端
  setvbuf(stdout, NULL, _IONBF, 0);

  // 设置全局配置文件路径
  rocket::Config::SetGlobalConfig("../conf/rocket.xml");

  // 初始化全局日志记录器
  rocket::Logger::InitGlobalLogger();

  // 调用测试连接函数
  // test_connect();

  // 调用测试 TcpClient 函数
  test_tcp_client();

  return 0;
}
