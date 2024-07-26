#ifndef ROCKET_NET_EVENTLOOP_H
#define ROCKET_NET_EVENTLOOP_H

#include <pthread.h>
#include <set>
#include <functional>
#include <queue>
#include <unistd.h>  // 包含 close 和 read 函数
#include "rocket/common/mutex.h"
#include "rocket/net/fd_event.h"
#include "rocket/net/wakeup_fd_event.h"
#include "rocket/net/timer.h"

using namespace std;

namespace rocket {

// EventLoop 类，负责管理事件循环
class EventLoop {
  public:
    // 构造函数
    EventLoop();

    // 析构函数
    ~EventLoop();

    // 事件循环主函数
    void loop();

    // 唤醒事件循环
    void wakeup();

    // 停止事件循环
    void stop();

    // 添加事件到 epoll
    void addEpollEvent(FdEvent* event);

    // 从 epoll 删除事件
    void deteteEpollEvent(FdEvent* event);

    // 检查是否在事件循环的线程中
    bool isInLoopThread();
    
    // 添加任务到事件循环
    void addTask(function<void()> cb, bool is_wake_up = false);

    void addTimerEvent(TimerEvent::s_ptr evnet);

    static EventLoop* GetCurrentEventLoop();

  private:
    // 事件循环所属线程 ID
    pid_t m_thread_id {0};

    // epoll 文件描述符
    int m_epoll_fd {0};

    // 唤醒文件描述符
    int m_wakeup_fd {0};

    // 唤醒事件对象
    WakeUpFdEvent* m_wakeup_fd_event {NULL};

    // 停止标志
    bool m_stop_flag {false};

    // 监听的文件描述符集合
    set<int> m_listen_fds;

    // 待处理的任务队列
    queue<function<void()>> m_pending_tasks;

    // 互斥锁，用于保护任务队列
    Mutex m_mutex;

    Timer* m_timer {NULL};

    // 处理唤醒事件
    void dealWakeup();

    // 初始化唤醒事件
    void initWakeUpFdEvent();

    void initTimer();

};

}

#endif
