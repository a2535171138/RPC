#ifndef ROCKET_NET_TCP_TCP_CONNECTION_H
#define ROCKET_NET_TCP_TCP_CONNECTION_H

#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/tcp/tcp_buffer.h"
#include "rocket/net/io_thread.h"

namespace rocket {

  // 表示TCP连接的状态
  enum TcpState {
    NotConnected = 1, // 未连接
    Connected = 2,    // 已连接
    HalfClosing = 3,  // 半关闭（正在关闭但未完全关闭）
    Closed = 4,       // 已关闭
  };

  class TcpConnection {
    public:
      typedef shared_ptr<TcpConnection> s_ptr;

      // 构造函数，初始化TCP连接
      TcpConnection(IOThread* io_thread, int fd, int buffer_size, NetAddr::s_ptr peer_addr);

      // 析构函数，清理TCP连接资源
      ~TcpConnection();

      // 读取数据
      void onRead();

      // 执行逻辑操作
      void excute();

      // 写入数据
      void onWrite();

      // 设置TCP连接的状态
      void setState(const TcpState state);

      // 获取TCP连接的状态
      TcpState getState();

      // 清理TCP连接
      void clear();

      // 服务器主动关闭连接
      void shutdown();

    private:
      IOThread* m_io_thread {NULL};           // IO线程

      NetAddr::s_ptr m_local_addr;            // 本地地址
      NetAddr::s_ptr m_peer_addr;             // 对端地址

      TcpBuffer::s_ptr m_in_buffer;           // 输入缓冲区
      TcpBuffer::s_ptr m_out_buffer;          // 输出缓冲区

      FdEvent* m_fd_event {NULL};             // 文件描述符事件

      TcpState m_state;                       // TCP连接的状态

      int m_fd {0};                           // 文件描述符
  };

}

#endif
