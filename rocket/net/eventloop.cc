#include <sys/socket.h>       // 提供 socket 操作的接口
#include <sys/epoll.h>        // 提供 epoll 相关操作的接口
#include <sys/eventfd.h>      // 提供 eventfd 操作的接口
#include <string.h>           // 提供字符串操作函数
#include "rocket/net/eventloop.h" // 引入 EventLoop 类的头文件
#include "rocket/common/log.h"    // 引入日志记录接口
#include "rocket/common/util.h"   // 引入工具函数（如线程ID获取）

// 定义将事件添加到 epoll 的宏
#define ADD_TO_EPOLL() \
    auto it = m_listen_fds.find(event->getFd()); \
    int op = EPOLL_CTL_ADD; \
    if(it != m_listen_fds.end()){ \
      op = EPOLL_CTL_MOD; \
    } \
    epoll_event tmp = event -> getEpollEvent(); \
    int rt = epoll_ctl(m_epoll_fd, op, event->getFd(), &tmp); \
    if(rt == -1){ \
      ERRORLOG("failed epoll_ctl when add fd %d, errno=%d, error=%s", errno, strerror(errno)); \
    } \
    DEBUGLOG("add event success, fd[%d]", event->getFd()) \

// 定义将事件删除出 epoll 的宏
#define DELETE_TO_EPOLL() \
    auto it = m_listen_fds.find(event->getFd()); \
    if(it == m_listen_fds.end()){ \
      return; \
    } \
    int op = EPOLL_CTL_DEL; \
    epoll_event tmp = event -> getEpollEvent(); \
    int rt = epoll_ctl(m_epoll_fd, op, event->getFd(), &tmp); \
    if(rt == -1){ \
      ERRORLOG("failed epoll_ctl when add fd %d, errno=%d, error=%s", errno, strerror(errno)); \
    } \
    DEBUGLOG("delete event success, fd[%d]", event->getFd()) \


namespace rocket {

// 线程局部存储的 EventLoop 指针，确保每个线程只有一个 EventLoop 实例
static thread_local EventLoop* t_current_eventloop = NULL;
// epoll 最大超时时间（毫秒）
static int g_epoll_max_timeout = 10000;
// epoll 最大事件数
static int g_epoll_max_events = 10;

// EventLoop 构造函数
EventLoop::EventLoop(){
  // 确保每个线程只能创建一个 EventLoop 实例
  if(t_current_eventloop != NULL){
    ERRORLOG("failed to create event loop, this thread has created event loop.");
    exit(0);
  }
  // 获取当前线程ID
  m_thread_id = getThreadId();

  // 创建 epoll 文件描述符
  m_epoll_fd = epoll_create(10);
  // 检查 epoll 创建是否成功
  if(m_epoll_fd == -1){
    ERRORLOG("failed to create event loop, epoll_craete error, error info[%d]", errno);
    exit(0);
  }

  // 初始化唤醒事件
  initWakeUpFdEvent();
  INFOLOG("succ create event loop in thread %d", m_thread_id);
  // 将当前 EventLoop 实例设置为线程局部存储
  t_current_eventloop = this;
}

// EventLoop 析构函数
EventLoop::~EventLoop(){
  // 关闭 epoll 文件描述符
  close(m_epoll_fd);
  // 释放唤醒事件对象
  if(m_wakeup_fd_event){
    delete m_wakeup_fd_event;
    m_wakeup_fd_event = NULL;
  }
}

// 初始化唤醒事件
void EventLoop::initWakeUpFdEvent(){
  // 创建 eventfd 文件描述符，用于事件唤醒，非阻塞模式
  m_wakeup_fd = eventfd(0, EFD_NONBLOCK);
  // 检查 eventfd 创建是否成功
  if(m_wakeup_fd < 0){
    ERRORLOG("failed to create event loop, eventfd create error, error info[%d]",  errno);
    exit(0);
  }

  // 创建 WakeUpFdEvent 对象并设置其回调函数
  m_wakeup_fd_event = new WakeUpFdEvent(m_wakeup_fd);
  m_wakeup_fd_event->listen(FdEvent::IN_EVENT, [this](){
    char buf[8];
    // 读取 eventfd 中的数据（用于唤醒事件）
    while(read(m_wakeup_fd, buf, 8) != -1 && errno != EAGAIN){
      DEBUGLOG("read full bytes from wakeup fd[%d]", m_wakeup_fd); 
    }
  });

  // 将唤醒事件添加到 epoll 事件中
  addEpollEvent(m_wakeup_fd_event);
}

// 事件循环主函数
void EventLoop::loop(){
  while(!m_stop_flag){
    // 锁定互斥量以访问待处理任务
    ScopeMutex<Mutex> lock(m_mutex);
    queue<function<void()>> temp_task;
    // 交换任务队列
    m_pending_tasks.swap(temp_task);
    lock.unlock();

    // 执行任务队列中的任务
    while(!temp_task.empty()){
      function<void()> cb = temp_task.front();
      temp_task.pop();
      if(cb){
        cb();
      }
    }

    // 设置 epoll 等待的超时时间
    int timeout = g_epoll_max_timeout; 
    epoll_event result_events[g_epoll_max_events]; // 用于存储 epoll 事件的数组
    DEBUGLOG("now begin to epoll_wait");
    // 调用 epoll_wait 等待事件发生
    int rt = epoll_wait(m_epoll_fd, result_events, g_epoll_max_events, timeout);
    DEBUGLOG("now end epoll_wait, rt = %d", rt);

    // 检查 epoll_wait 是否成功
    if(rt < 0){
      ERRORLOG("epoll_wait error, errno=%d", errno);
    }
    else {
      // 处理 epoll_wait 返回的事件
      for(int i=0; i < rt; ++i){
        epoll_event trigger_event = result_events[i];
        FdEvent* fd_event = static_cast<FdEvent*>(trigger_event.data.ptr);
        if(fd_event == NULL){
          continue;
        }

        // 处理触发的读事件
        if(trigger_event.events & EPOLLIN){
          DEBUGLOG("fd %d trigger EPOLLIN event", fd_event->getFd());
          addTask(fd_event->handler(FdEvent::IN_EVENT));
        }
        // 处理触发的写事件
        if(trigger_event.events & EPOLLOUT){
          DEBUGLOG("fd %d trigger EPOLLOUT event", fd_event->getFd());
          addTask(fd_event->handler(FdEvent::OUT_EVENT));
        }
      }
    }
  }
}

// 唤醒事件循环
void EventLoop::wakeup(){
  INFOLOG("WAKE UP")
  m_wakeup_fd_event->wakeup();
}

// 停止事件循环
void EventLoop::stop(){
  m_stop_flag = true;
}

// 添加 epoll 事件
void EventLoop::addEpollEvent(FdEvent* event){
  // 如果当前在事件循环线程中，直接添加事件
  if(isInLoopThread()){
    ADD_TO_EPOLL();
  }
  else {
    // 否则，将添加事件的操作加入任务队列，并唤醒事件循环
    auto cb = [this, event](){
      ADD_TO_EPOLL();
    };
    addTask(cb, true);
  }
}

// 删除 epoll 事件
void EventLoop::deteteEpollEvent(FdEvent* event){
  // 如果当前在事件循环线程中，直接删除事件
  if(isInLoopThread()){
    DELETE_TO_EPOLL();
  }
  else {
    // 否则，将删除事件的操作加入任务队列，并唤醒事件循环
    auto cb = [this, event](){
      DELETE_TO_EPOLL();
    };
    addTask(cb, true);
  }
}

// 添加任务到事件循环
void EventLoop::addTask(function<void()> cb, bool is_wake_up){
  // 锁定互斥量以访问待处理任务队列
  ScopeMutex<Mutex> lock(m_mutex);
  // 将任务添加到队列中
  m_pending_tasks.push(cb);
  lock.unlock();
}

// 检查当前是否在事件循环线程中
bool EventLoop::isInLoopThread(){
  return getThreadId() == m_thread_id;
}

}
