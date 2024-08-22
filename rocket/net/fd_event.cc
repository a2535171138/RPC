#include <string.h>
#include <fcntl.h>
#include "rocket/net/fd_event.h"
#include "rocket/common/log.h"

namespace rocket {

  // FdEvent类的构造函数，初始化文件描述符和监听事件
  FdEvent::FdEvent(int fd) : m_fd(fd) {
    // 将监听事件结构体清零
    memset(&m_listen_events, 0, sizeof(m_listen_events));
  }

  // 默认构造函数，初始化监听事件
  FdEvent::FdEvent() {
    // 将监听事件结构体清零
    memset(&m_listen_events, 0, sizeof(m_listen_events));
  }

  // FdEvent类的析构函数，目前未做任何操作
  FdEvent::~FdEvent() {
    // 可在此添加资源清理代码，如关闭文件描述符等
  }

  // 根据事件类型返回相应的回调函数
  function<void()> FdEvent::handler(TriggerEvent event) {
    if (event == TriggerEvent::IN_EVENT) {
      // 如果是读事件，返回读回调函数
      return m_read_callback;
    } else if(event == TriggerEvent::OUT_EVENT){
      // 如果是写事件，返回写回调函数
      return m_write_callback;
    } else if(event == TriggerEvent::ERROR_EVENT){
      return m_error_callback;
    }
    return nullptr;
  }

  // 设置指定事件类型的回调函数，并配置epoll监听的事件类型
  void FdEvent::listen(TriggerEvent event_type, function<void()> callback, function<void()> error_callback) {
    if (event_type == TriggerEvent::IN_EVENT) {
      // 如果是读事件，设置读回调函数，并将事件类型设置为EPOLLIN
      m_listen_events.events |= EPOLLIN;
      m_read_callback = callback;
    } else {
      // 如果是写事件，设置写回调函数，并将事件类型设置为EPOLLOUT
      m_listen_events.events |= EPOLLOUT;
      m_write_callback = callback;
    }

    if(m_error_callback == nullptr){
      m_error_callback = error_callback;
    }
    else {
      m_error_callback = nullptr;
    }

    // 将当前对象的指针存储到监听事件的数据中
    m_listen_events.data.ptr = this;
  }

  // 取消指定事件类型的监听
  void FdEvent::cancel(TriggerEvent event_type) {
    if (event_type == TriggerEvent::IN_EVENT) {
      // 取消读事件监听
      m_listen_events.events &= (~EPOLLIN);
    } else {
      // 取消写事件监听
      m_listen_events.events &= (~EPOLLOUT);
    }
  }

  // 设置文件描述符为非阻塞模式
  void FdEvent::setNonBlock() {
    int flag = fcntl(m_fd, F_GETFL, 0); // 获取文件描述符的当前状态标志
    if (flag & O_NONBLOCK) {
      return; // 如果已经是非阻塞模式，则直接返回
    }
    fcntl(m_fd, F_SETFL, flag | O_NONBLOCK); // 设置文件描述符为非阻塞模式
  }

}
