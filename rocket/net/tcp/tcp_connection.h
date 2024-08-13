#ifndef ROCKET_NET_TCP_TCP_CONNECTION_H
#define ROCKET_NET_TCP_TCP_CONNECTION_H

#include <memory>
#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/tcp/tcp_buffer.h"
#include "rocket/net/io_thread.h"
#include "rocket/net/coder/abstract_coder.h"
#include "rocket/net/rpc/rpc_dispatcher.h"

namespace rocket {

// 表示 TCP 连接的状态
enum TcpState {
    NotConnected = 1, // 未连接
    Connected = 2,    // 已连接
    HalfClosing = 3,  // 半关闭（正在关闭但未完全关闭）
    Closed = 4,       // 已关闭
};

// 表示 TCP 连接的类型
enum TcpConnectionType {
    TcpConnectionByServer = 1,  // 作为服务端使用，代表与对端客户端的连接
    TcpConnectionByClient = 2,  // 作为客户端使用，代表与对端服务端的连接
};

// TcpConnection 类，表示一个 TCP 连接
class TcpConnection {
  public:
    typedef std::shared_ptr<TcpConnection> s_ptr; // 智能指针类型定义

    // 构造函数，初始化 TCP 连接
    TcpConnection(EventLoop* event_loop, int fd, int buffer_size, NetAddr::s_ptr peer_addr, NetAddr::s_ptr local_addr, TcpConnectionType type = TcpConnectionByServer);

    // 析构函数，清理 TCP 连接资源
    ~TcpConnection();

    // 读取数据，处理从连接中读取到的数据
    void onRead();

    // 执行逻辑操作，处理业务逻辑
    void excute();

    // 写入数据，将数据写入连接
    void onWrite();

    // 设置 TCP 连接的状态
    void setState(const TcpState state);

    // 获取 TCP 连接的状态
    TcpState getState();

    // 清理 TCP 连接，释放资源
    void clear();

    // 服务器主动关闭连接
    void shutdown();

    // 设置 TCP 连接的类型
    void setConnectionType(TcpConnectionType type);

    // 启动监听可写事件
    void listenWrite();

    // 启动监听可读事件
    void listenRead();

    // 推送要发送的消息到队列
    void pushSendMessage(AbstractProtocol::s_ptr message, std::function<void(AbstractProtocol::s_ptr)> done);

    // 推送要读取的消息到队列
    void pushReadMessage(const std::string& msg_id, std::function<void(AbstractProtocol::s_ptr)> done);

    NetAddr::s_ptr getLocalAddr();

    NetAddr::s_ptr getPeerAddr();

  private:
    EventLoop* m_event_loop {NULL};             // 事件循环对象指针

    NetAddr::s_ptr m_local_addr;                // 本地地址
    NetAddr::s_ptr m_peer_addr;                 // 对端地址

    TcpBuffer::s_ptr m_in_buffer;               // 输入缓冲区
    TcpBuffer::s_ptr m_out_buffer;              // 输出缓冲区

    FdEvent* m_fd_event {NULL};                 // 文件描述符事件对象

    AbstractCoder* m_coder {NULL};              // 编解码器对象

    TcpState m_state;                          // TCP 连接的状态

    int m_fd {0};                              // 文件描述符

    TcpConnectionType m_connection_type {TcpConnectionByServer}; // TCP 连接的类型

    std::vector<std::pair<AbstractProtocol::s_ptr, std::function<void(AbstractProtocol::s_ptr)>>> m_write_dones; // 写操作完成的回调函数

    std::map<std::string, std::function<void(AbstractProtocol::s_ptr)>> m_read_dones; // 读操作完成的回调函数

};

}

#endif
