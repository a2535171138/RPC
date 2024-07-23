#include <pthread.h>               // 引入 POSIX 线程库
#include <sys/socket.h>            // 引入 socket 操作相关的函数
#include <netinet/in.h>            // 引入 IP 地址和端口操作相关的结构
#include <arpa/inet.h>             // 引入 IP 地址转换函数
#include <string.h>                // 引入字符串操作函数
#include <unistd.h>                // 引入 Unix 标准函数，如 close
#include <cstdio>                  // 引入 C 标准输入输出库
#include <memory>                  
#include "rocket/common/log.h"     // 引入自定义日志库
#include "rocket/common/config.h"  // 引入自定义配置库
#include "rocket/net/fd_event.h"   // 引入 FdEvent 类的头文件
#include "rocket/net/eventloop.h"  // 引入 EventLoop 类的头文件
#include "rocket/net/timer_event.h" 
#include "rocket/net/io_thread.h"
#include "rocket/net/io_thread_group.h"

void test_io_thread() {
  // 创建一个监听套接字，用于接受客户端连接
  int listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if (listenfd == -1) {
    // 如果套接字创建失败，记录错误日志并退出程序
    ERRORLOG("listenfd = -1");
    exit(0);
  }

  // 定义并初始化 sockaddr_in 结构体，用于绑定 IP 地址和端口
  sockaddr_in addr;
  memset(&addr, 0, sizeof(addr)); // 清零结构体

  addr.sin_port = htons(12369); // 设置监听端口
  addr.sin_family = AF_INET;    // 设置地址族为 IPv4

  // 将 IP 地址转换为网络字节序，并赋值给 addr.sin_addr
  inet_aton("127.0.0.1", &addr.sin_addr);

  // 绑定监听套接字到指定的地址和端口
  int rt = bind(listenfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
  if (rt != 0) {
    // 如果绑定失败，记录错误日志并退出程序
    ERRORLOG("bind error");
    exit(1);
  }

  // 开始监听套接字，最大连接数为 100
  rt = listen(listenfd, 100);
  if (rt != 0) {
    // 如果监听失败，记录错误日志并退出程序
    ERRORLOG("listen error");
    exit(1);
  }

  // 创建 FdEvent 实例，用于监听套接字的事件
  rocket::FdEvent event(listenfd);
  // 注册事件回调函数，当有 IN_EVENT 事件（即可读事件）发生时调用
  event.listen(rocket::FdEvent::IN_EVENT, [listenfd]() {
    sockaddr_in peer_addr;
    socklen_t addr_len = sizeof(peer_addr);
    memset(&peer_addr, 0, sizeof(peer_addr));
    // 接受客户端连接，并获取客户端的套接字描述符
    int clientfd = accept(listenfd, reinterpret_cast<sockaddr*>(&peer_addr), &addr_len);

    // 将客户端 IP 地址转换为点分十进制字符串
    const char* client_ip = inet_ntoa(peer_addr.sin_addr);
    // 记录成功接收到客户端连接的信息
    DEBUGLOG("success get client fd[%d], peer addr: [%s:%d]", clientfd, client_ip, ntohs(peer_addr.sin_port));
  });

  // 创建一个计时器事件，每隔 1000 毫秒触发一次
  int i = 0;
  rocket::TimerEvent::s_ptr timer_event = make_shared<rocket::TimerEvent>(
    1000, true, [&i]() {
      INFOLOG("trigger timer event, count=%d", i++);
    }
  );

  // 创建一个 I/O 线程组，包含 2 个线程
  rocket::IOThreadGroup io_thread_group(2);

  // 获取并配置第一个 I/O 线程
  rocket::IOThread* io_thread = io_thread_group.getIOThread();
  io_thread->getEventLoop()->addEpollEvent(&event); // 将 FdEvent 添加到事件循环中
  io_thread->getEventLoop()->addTimerEvent(timer_event); // 将计时器事件添加到事件循环中

  // 获取并配置第二个 I/O 线程
  rocket::IOThread* io_thread2 = io_thread_group.getIOThread();
  io_thread2->getEventLoop()->addTimerEvent(timer_event); // 将计时器事件添加到第二个线程的事件循环中

  // 启动 I/O 线程组
  io_thread_group.start();
  // 等待 I/O 线程组中的所有线程完成
  io_thread_group.join();
}

int main() {
  // 禁用 stdout 的缓冲区，这样 printf 直接输出到终端
  setvbuf(stdout, NULL, _IONBF, 0);

  // 加载全局配置
  rocket::Config::SetGlobalConfig("../conf/rocket.xml");

  // 初始化全局日志记录器
  rocket::Logger::InitGlobalLogger();

  // 执行测试函数
  test_io_thread();

  return 0;
}
