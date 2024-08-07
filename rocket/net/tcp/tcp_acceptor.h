#ifndef ROCKET_NET_TCP_TCP_ADDR_H
#define ROCKET_NET_TCP_TCP_ADDR_H

#include <memory>  // 引入标准库中的memory头文件
#include "rocket/net/tcp/net_addr.h"  // 引入NetAddr头文件

namespace rocket {  // 定义命名空间rocket

class TcpAcceptor {  // 定义TcpAcceptor类
  public:
    typedef shared_ptr<TcpAcceptor> s_ptr;  // 定义一个智能指针类型，指向TcpAcceptor

    TcpAcceptor(NetAddr::s_ptr local_addr);  // 构造函数，初始化本地地址

    ~TcpAcceptor();  // 析构函数，释放资源

    pair<int, NetAddr::s_ptr> accept();  // 接受连接

    int getListnFd();  // 获取监听文件描述符

  private:
    NetAddr::s_ptr m_local_addr;  // 本地地址

    int m_family {-1};  // 地址族，初始值为-1

    int m_listenfd {-1};  // 监听文件描述符，初始值为-1
};

}

#endif  // 结束预处理指令，防止重复包含
