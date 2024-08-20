#ifndef ROCKET_NET_TCP_TCP_CLIENT_H
#define ROCKET_NET_TCP_TCP_CLIENT_H

#include <memory>
#include "rocket/net/tcp/net_addr.h"         // 提供网络地址的定义
#include "rocket/net/eventloop.h"            // 提供事件循环功能
#include "rocket/net/tcp/tcp_connection.h"   // 提供 TCP 连接的定义
#include "rocket/net/coder/abstract_protocol.h"  // 提供抽象协议的定义

namespace rocket {

// TcpClient 类定义
class TcpClient {
public:
    typedef shared_ptr<TcpClient> s_ptr;

    // 构造函数，初始化 TcpClient 对象
    TcpClient(NetAddr::s_ptr peer_addr);

    // 析构函数，释放资源
    ~TcpClient();
    
    // 异步连接函数
    // 如果连接成功，将执行 done 函数
    void connect(std::function<void()> done);

    // 异步发送消息函数
    // 如果消息发送成功，将调用 done 函数，函数的参数为消息对象
    void writeMessage(AbstractProtocol::s_ptr message, std::function<void(AbstractProtocol::s_ptr)> done);

    // 异步读取消息函数
    // 如果成功读取消息，将调用 done 函数，函数的参数为消息对象
    void readMessage(const std::string& msg_id, std::function<void(AbstractProtocol::s_ptr)> done);

    void stop();

private:
    NetAddr::s_ptr m_peer_addr;             // 存储对端地址
    EventLoop* m_event_loop {nullptr};      // 存储事件循环对象指针

    int m_fd {-1};                         // 存储文件描述符
    FdEvent* m_fd_event {nullptr};         // 存储文件描述符事件对象指针

    TcpConnection::s_ptr m_connection;    // 存储 TCP 连接对象指针
};

}

#endif
