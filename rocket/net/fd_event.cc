#include <string.h>
#include "rocket/net/fd_event.h"
#include "rocket/common/log.h"

namespace rocket {

// FdEvent类的构造函数，初始化文件描述符和监听事件
FdEvent::FdEvent(int fd) : m_fd(fd) {
  // 将监听事件结构体清零
  memset(&m_listen_events, 0, sizeof(m_listen_events));
}

// FdEvent类的析构函数，目前未做任何操作
FdEvent::~FdEvent(){

}

// 根据事件类型返回相应的回调函数
function<void()> FdEvent::handler(TriggerEvent event){
  if(event == TriggerEvent::IN_EVENT){
    // 如果是读事件，返回读回调函数
    return m_read_callback;
  }
  else{
    // 如果是写事件，返回写回调函数
    return m_write_callback;
  }
}

// 设置指定事件类型的回调函数，并配置epoll监听的事件类型
void FdEvent::listen(TriggerEvent event_type, function<void()> callback){
  if(event_type == TriggerEvent::IN_EVENT){
    // 如果是读事件，设置读回调函数，并将事件类型设置为EPOLLIN
    m_listen_events.events |= EPOLLIN;
    m_read_callback = callback;
  }
  else {
    // 如果是写事件，设置写回调函数，并将事件类型设置为EPOLLOUT
    m_listen_events.events |= EPOLLOUT;
    m_write_callback = callback;
  }

  // 将当前对象的指针存储到监听事件的数据中
  m_listen_events.data.ptr = this;
}

}
