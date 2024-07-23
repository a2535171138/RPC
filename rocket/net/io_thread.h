#ifndef ROCKET_NET_IO_THREAD_H
#define ROCKET_NET_IO_THREAD_H

#include <pthread.h>      // POSIX 线程库头文件
#include <semaphore.h>    // POSIX 信号量库头文件
#include "rocket/net/eventloop.h" // 包含事件循环类的头文件

namespace rocket {

// IOThread 类用于管理单个 I/O 线程及其事件循环
class IOThread {
  public:
    IOThread();  // 构造函数，初始化 I/O 线程

    ~IOThread(); // 析构函数，清理 I/O 线程资源

    EventLoop* getEventLoop(); // 获取 I/O 线程的事件循环对象指针

    void start(); // 启动 I/O 线程

    void join();  // 等待 I/O 线程执行完毕

    // 静态成员函数，线程的主执行函数
    static void* Main(void* arg);

  private:
    pid_t m_thread_id {-1};  // 线程 ID，初始化为 -1
    pthread_t m_thread {0};  // POSIX 线程对象，初始化为 0

    EventLoop* m_event_loop {NULL}; // 事件循环对象指针，初始化为 NULL

    sem_t m_init_semaphore;  // 初始化信号量，用于同步线程的初始化过程
    sem_t m_start_semaphore; // 启动信号量，用于同步线程的启动过程

};

} // namespace rocket

#endif // ROCKET_NET_IO_THREAD_H
