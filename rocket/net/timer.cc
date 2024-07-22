#include <sys/timerfd.h>
#include <string.h>
#include <unistd.h>
#include "rocket/net/timer.h"
#include "rocket/common/log.h"
#include "rocket/common/util.h"

namespace rocket {

// Timer 类的构造函数
Timer::Timer() : FdEvent() {
  // 创建定时器文件描述符，使用单调时钟，非阻塞和执行时关闭标志
  m_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

  // 打印调试信息，显示定时器文件描述符
  DEBUGLOG("time fd=%d", m_fd);

  // 监听定时器事件，将 IN_EVENT 事件与 onTimer 函数绑定
  listen(FdEvent::IN_EVENT, bind(&Timer::onTimer, this));
}

// Timer 类的析构函数
Timer::~Timer() {
  // 在这里可以添加资源清理代码，但目前没有需要显式清理的资源
}

// 定时器事件的处理函数
void Timer::onTimer() {
  char buf[8]; // 用于读取定时器文件描述符的数据缓冲区
  while (1) {
    // 读取定时器文件描述符，以清除其可读事件
    if ((read(m_fd, buf, 8) == -1) && errno == EAGAIN) {
      // 如果读取返回 EAGAIN，表示没有更多数据可读，跳出循环
      break;
    }
  }

  // 获取当前时间（毫秒）
  int64_t now = getNowMs();

  // 临时存储需要重置的定时器事件
  vector<TimerEvent::s_ptr> tmps;
  // 存储需要执行的任务，包含任务到达时间和回调函数
  vector<pair<int64_t, function<void()>>> tasks;

  // 使用互斥锁保护共享数据
  ScopeMutex<Mutex> lock(m_mutex);
  auto it = m_pending_events.begin();

  // 遍历所有待处理事件，查找所有到达时间小于等于当前时间的事件
  for (it = m_pending_events.begin(); it != m_pending_events.end(); ++it) {
    if ((*it).first <= now) {
      // 如果事件未被取消，将其添加到临时存储和任务列表中
      if (!(*it).second->isCanceled()) {
        tmps.push_back((*it).second);
        tasks.push_back(make_pair((*it).second->getArriveTime(), (*it).second->getCallBack()));
      }
    } else {
      // 如果找到一个事件的到达时间大于当前时间，跳出循环
      break;
    }
  }

  // 删除已经处理过的定时器事件
  m_pending_events.erase(m_pending_events.begin(), it);
  lock.unlock(); // 解锁

  // 遍历需要重置的定时器事件
  for (auto i = tmps.begin(); i != tmps.end(); ++i) {
    if ((*i)->isRepeated()) {
      // 如果事件是重复的，重置其到达时间并重新添加到事件集合中
      (*i)->resetArriveTime();
      addTimerEvent(*i);
    }
  }

  // 重置定时器的到达时间
  resetArriveTime();

  // 执行所有到达的任务
  for (auto i : tasks) {
    if (i.second) {
      // 执行回调函数
      i.second();
    }
  }
}

// 重置定时器的到达时间
void Timer::resetArriveTime() {
  // 使用互斥锁保护共享数据
  ScopeMutex<Mutex> lock(m_mutex);
  auto tmp = m_pending_events;
  lock.unlock();

  // 如果没有待处理事件，直接返回
  if (tmp.size() == 0) {
    return;
  }

  // 获取当前时间（毫秒）
  int64_t now = getNowMs();

  auto it = tmp.begin();
  int64_t interval = 0;
  // 计算下一个定时器事件的触发时间间隔
  if (it->second->getArriveTime() > now) {
    interval = it->second->getArriveTime() - now;
  } else {
    interval = 100;
  }

  timespec ts;
  memset(&ts, 0, sizeof(ts));
  ts.tv_sec = interval / 1000; // 秒
  ts.tv_nsec = (interval % 1000) * 1000000; // 纳秒

  itimerspec value;
  memset(&value, 0, sizeof(value));
  value.it_value = ts;

  // 设置定时器的到达时间
  int rt = timerfd_settime(m_fd, 0, &value, NULL);
  if (rt != 0) {
    // 如果设置失败，记录错误日志
    ERRORLOG("timerfd_settime error, errno=%d, error=%s", errno, strerror(errno));
  }
  DEBUGLOG("timer reset to %lld", now + interval); // 打印调试信息
}

// 添加一个定时器事件
void Timer::addTimerEvent(TimerEvent::s_ptr event) {
  bool is_reset_timerfd = false; // 标记是否需要重置定时器

  // 使用互斥锁保护共享数据
  ScopeMutex<Mutex> lock(m_mutex);
  if (m_pending_events.empty()) {
    is_reset_timerfd = true; // 如果事件集合为空，需要重置定时器
  } else {
    auto it = m_pending_events.begin();
    if ((*it).second->getArriveTime() > event->getArriveTime()) {
      is_reset_timerfd = true; // 如果新事件的到达时间早于现有事件，需要重置定时器
    }
  }

  // 将事件添加到待处理事件集合中
  m_pending_events.emplace(event->getArriveTime(), event);
  lock.unlock();

  // 如果需要重置定时器，调用 resetArriveTime
  if (is_reset_timerfd) {
    resetArriveTime();
  }
}

// 删除一个定时器事件
void Timer::deteteTimerEvent(TimerEvent::s_ptr event) {
  event->setCanceled(true); // 标记事件为已取消

  // 使用互斥锁保护共享数据
  ScopeMutex<Mutex> lock(m_mutex);

  // 查找事件在集合中的位置
  auto begin = m_pending_events.lower_bound(event->getArriveTime());
  auto end = m_pending_events.upper_bound(event->getArriveTime());

  auto it = begin;
  for (it = begin; it != end; ++it) {
    if (it->second == event) {
      break;
    }
  }

  // 如果找到事件，删除它
  if (it != end) {
    m_pending_events.erase(it);
  }

  lock.unlock();

  // 打印调试信息，显示删除的定时器事件的到达时间
  DEBUGLOG("success delete TimerEvent at arrive time %lld", event->getArriveTime());
}

}
