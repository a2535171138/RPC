#ifndef ROCKET_NET_FDEVENT_H
#define ROCKET_NET_FDEVENT_H

#include <functional>
#include <sys/epoll.h>

using namespace std;

namespace rocket {

  // 定义一个文件描述符事件类，用于处理epoll事件
  class FdEvent {
    public:
      // 触发事件的类型
      enum TriggerEvent {
        IN_EVENT = EPOLLIN,   // 读事件
        OUT_EVENT = EPOLLOUT, // 写事件
      };

      // 构造函数，接受一个文件描述符
      FdEvent(int fd);

      // 默认构造函数
      FdEvent();

      // 析构函数，清理资源
      ~FdEvent();

      // 设置文件描述符为非阻塞模式
      void setNonBlock();

      // 获取事件处理函数，根据事件类型返回相应的回调函数
      function<void()> handler(TriggerEvent event_type);

      // 为指定的触发事件类型设置回调函数
      void listen(TriggerEvent event_type, function<void()> callback);

      // 取消指定事件类型的监听
      void cancel(TriggerEvent event_type);

      // 获取文件描述符
      int getFd() const {
        return m_fd;
      }

      // 获取epoll事件结构体
      epoll_event getEpollEvent() {
        return m_listen_events;
      }

    protected:
      int m_fd {-1}; // 文件描述符，默认值为-1

      epoll_event m_listen_events; // 用于epoll的事件结构

      function<void()> m_read_callback;  // 读事件回调函数
      function<void()> m_write_callback; // 写事件回调函数
  };

}

#endif
