#ifndef ROCKET_NET_TCP_SERVER_H
#define ROCKET_NET_TCP_SERVER_H

#include "rocket/net/tcp/tcp_acceptor.h"  // 引入TcpAcceptor头文件
#include "rocket/net/tcp/net_addr.h"  // 引入NetAddr头文件
#include "rocket/net/eventloop.h"  // 引入EventLoop头文件
#include "rocket/net/io_thread_group.h"  // 引入IOThreadGroup头文件

namespace rocket {  // 定义命名空间rocket

class TcpServer {  // 定义TcpServer类
  public:
    TcpServer(NetAddr::s_ptr local_addr);  // 构造函数，初始化本地地址

    ~TcpServer();  // 析构函数，释放资源

    void start();  // 启动服务器

  private:
    TcpAcceptor::s_ptr m_acceptor;  // 智能指针，指向TcpAcceptor

    NetAddr::s_ptr m_local_addr;  // 智能指针，指向本地地址

    EventLoop* m_main_event_loop {NULL};  // 主事件循环指针，初始化为NULL

    IOThreadGroup* m_io_thread_group {NULL};  // IO线程组指针，初始化为NULL

    FdEvent* m_listen_fd_event;  // 监听文件描述符事件指针

    int m_client_counts {0};  // 客户端连接计数，初始化为0

    void init();  // 初始化函数

    void onAccept();  // 处理接受连接的函数
};

}

#endif  // 结束预处理指令，防止重复包含
