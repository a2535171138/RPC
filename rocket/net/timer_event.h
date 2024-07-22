#ifndef ROCKET_NET_TIMEREVENT_H
#define ROCKET_NET_TIMEREVENT_H

#include <functional>
#include <memory>

using namespace std;

namespace rocket {

// 定义 TimerEvent 类，用于处理定时器事件
class TimerEvent {
  public:
    // 定义共享指针类型
    typedef shared_ptr<TimerEvent> s_ptr;

    // 构造函数，接受定时间隔、是否重复、以及回调函数
    TimerEvent(int interval, bool is_repeated, function<void()> cb);

    // 获取定时器事件到达时间
    int64_t getArriveTime() const {
      return m_arrive_time;
    }

    // 设置定时器事件是否取消
    void setCanceled(bool value) {
      m_is_canceled = value;
    }

    // 判断定时器事件是否取消
    bool isCanceled() {
      return m_is_canceled;
    }

    // 判断定时器事件是否重复
    bool isRepeated() {
      return m_is_repeated;
    }

    // 获取定时器事件的回调函数
    function<void()> getCallBack() {
      return m_task;
    }

    // 重置定时器事件到达时间
    void resetArriveTime();

  private:
    int64_t m_arrive_time;  // 定时器事件到达时间，单位为毫秒
    int64_t m_interval;     // 定时间隔，单位为毫秒
    bool m_is_repeated {false};  // 标记定时器事件是否重复
    bool m_is_canceled {false};  // 标记定时器事件是否取消

    function<void()> m_task;  // 定时器事件的回调函数
};

}

#endif
