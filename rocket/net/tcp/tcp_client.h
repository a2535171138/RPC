#ifndef ROCKET_NET_TCP_TCP_CLIENT_H
#define ROCKET_NET_TCP_TCP_CLIENT_H

#include <memory>
#include "rocket/net/tcp/net_addr.h"         // 提供网络地址的定义
#include "rocket/net/eventloop.h"            // 提供事件循环功能
#include "rocket/net/tcp/tcp_connection.h"   // 提供 TCP 连接的定义
#include "rocket/net/coder/abstract_protocol.h"  // 提供抽象协议的定义
#include "rocket/net/timer_event.h"          // 提供定时事件的定义

namespace rocket {

// TcpClient 类定义，表示一个 TCP 客户端，用于与远程服务器建立连接并进行通信
class TcpClient {
public:
    typedef std::shared_ptr<TcpClient> s_ptr;  // 定义智能指针类型，简化指针的使用

    // 构造函数，初始化 TcpClient 对象
    // 参数: peer_addr - 目标服务器的网络地址
    TcpClient(NetAddr::s_ptr peer_addr);

    // 析构函数，释放 TcpClient 使用的资源
    ~TcpClient();
    
    // 异步连接函数
    // 参数: done - 当连接成功时调用的回调函数
    void connect(std::function<void()> done);

    // 异步发送消息函数
    // 参数:
    // message - 要发送的消息，使用抽象协议表示
    // done - 当消息发送成功时调用的回调函数
    void writeMessage(AbstractProtocol::s_ptr message, std::function<void(AbstractProtocol::s_ptr)> done);

    // 异步读取消息函数
    // 参数:
    // msg_id - 消息的唯一标识符，用于匹配响应
    // done - 当成功读取到消息时调用的回调函数
    void readMessage(const std::string& msg_id, std::function<void(AbstractProtocol::s_ptr)> done);

    // 停止客户端的操作，关闭连接
    void stop();

    // 获取连接错误码
    // 返回值: 错误码，如果没有错误则返回 0
    int getConnectErrorCode();

    // 获取连接错误信息
    // 返回值: 错误信息的字符串描述
    std::string getConnectErrorInfo();

    // 获取对端（服务器）的网络地址
    // 返回值: 表示服务器地址的智能指针
    NetAddr::s_ptr getPeerAddr();
    
    // 获取本地的网络地址（客户端地址）
    // 返回值: 表示客户端地址的智能指针
    NetAddr::s_ptr getLocalAddr();

    // 初始化本地地址信息
    void initLocalAddr();

    // 添加定时事件
    // 参数: timer_event - 要添加的定时事件
    void addTimerEvent(TimerEvent::s_ptr timer_event);

private:
    NetAddr::s_ptr m_peer_addr;             // 存储对端（服务器）地址的智能指针
    NetAddr::s_ptr m_local_addr;            // 存储本地（客户端）地址的智能指针

    EventLoop* m_event_loop {NULL};         // 存储事件循环对象指针，用于管理事件的注册和处理

    int m_fd {-1};                          // 存储文件描述符，用于标识与服务器的连接
    FdEvent* m_fd_event {NULL};             // 存储文件描述符事件对象指针，用于管理文件描述符上的事件

    TcpConnection::s_ptr m_connection;      // 存储 TCP 连接对象指针，用于管理与服务器的连接

    int m_connect_error_code = {0};         // 连接错误码，0 表示无错误
    std::string m_connect_error_info;       // 连接错误信息的字符串描述
};

} // namespace rocket

#endif // ROCKET_NET_TCP_TCP_CLIENT_H
