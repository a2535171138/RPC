#ifndef ROCKET_NET_WAKEUP_FDEVENT_H
#define ROCKET_NET_WAKEUP_FDEVENT_H

#include "rocket/net/fd_event.h"

namespace rocket{

// WakeUpFdEvent类继承自FdEvent类
class WakeUpFdEvent : public FdEvent {
  public:
    // 构造函数，初始化基类FdEvent
    WakeUpFdEvent(int fd);

    // 析构函数
    ~WakeUpFdEvent();

    // 唤醒函数
    void wakeup();

  private:
    // 目前没有私有成员变量

};

}

#endif
