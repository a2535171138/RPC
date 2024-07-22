#include "rocket/net/timer_event.h"
#include "rocket/common/log.h"
#include "rocket/common/util.h"

namespace rocket {

// 构造函数，初始化定时间隔、是否重复执行和回调函数，并重置到达时间
TimerEvent::TimerEvent(int interval, bool is_repeated, function<void()> cb) 
: m_interval(interval), m_is_repeated(is_repeated), m_task(cb) {
  resetArriveTime();  // 调用重置到达时间的方法
}

// 重置到达时间的方法，将当前时间加上定时间隔作为新的到达时间
void TimerEvent::resetArriveTime() {
  m_arrive_time = getNowMs() + m_interval;  // 获取当前时间并加上定时间隔
  DEBUGLOG("success create timer event, will exute at [%lld]", m_arrive_time);  // 记录日志，输出到达时间
}

}
