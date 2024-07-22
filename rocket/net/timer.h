#ifndef ROCKET_NET_TIMER_H
#define ROCKET_NET_TIMER_H

#include <map>
#include "rocket/net/fd_event.h"
#include "rocket/net/timer_event.h"
#include "rocket/common/mutex.h"

namespace rocket {

// Timer 类继承自 FdEvent，管理定时器事件
class Timer : public FdEvent {
  public:
    // 构造函数，初始化定时器
    Timer();
    
    // 析构函数，清理资源
    ~Timer();

    // 添加一个定时器事件
    void addTimerEvent(TimerEvent::s_ptr event);

    // 删除一个定时器事件
    void deteteTimerEvent(TimerEvent::s_ptr event);

    // 定时器事件处理函数
    void onTimer();

  private:
    // 使用 multimap 存储待处理的定时器事件，key 为到达时间，value 为 TimerEvent 的共享指针
    multimap<int64_t, TimerEvent::s_ptr> m_pending_events;
    
    // 互斥锁，用于保护定时器事件的线程安全访问
    Mutex m_mutex;

    // 重置到达时间的方法
    void resetArriveTime();
};

}

#endif
