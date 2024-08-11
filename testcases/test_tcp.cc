#include <memory>  // 引入标准库中的memory头文件
#include "rocket/common/log.h"  // 引入日志头文件
#include "rocket/net/tcp/net_addr.h"  // 引入NetAddr头文件
#include "rocket/net/tcp/tcp_server.h"  // 引入TcpServer头文件

void test_tcp_server() {  // 定义测试TCP服务器的函数
  rocket::IPNetAddr::s_ptr addr = make_shared<rocket::IPNetAddr>("127.0.0.1", 12351);  // 创建IPNetAddr对象，指定IP地址和端口号

  DEBUGLOG("create addr %s", addr->toString().c_str());  // 记录调试日志，显示创建的地址

  rocket::TcpServer tcp_server(addr);  // 创建TcpServer对象

  tcp_server.start();  // 启动TCP服务器
}

int main() {  // 主函数
  rocket::Config::SetGlobalConfig("../conf/rocket.xml");  // 设置全局配置
  rocket::Logger::InitGlobalLogger();  // 初始化全局日志系统

  test_tcp_server();  // 测试TCP服务器
}
