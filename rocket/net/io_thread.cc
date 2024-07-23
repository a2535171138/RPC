#include <pthread.h> // POSIX 线程库头文件
#include "rocket/net/io_thread.h" // IOThread 类的头文件
#include "rocket/common/log.h" // 日志功能头文件
#include "rocket/common/util.h" // 通用工具头文件
#include <cassert> // 断言库头文件

namespace rocket{

// IOThread 类的构造函数，初始化 I/O 线程及其资源
IOThread::IOThread(){
  // 初始化用于线程初始化过程的信号量
  int rt = sem_init(&m_init_semaphore, 0, 0);
  assert(rt == 0);

  // 初始化用于线程启动过程的信号量
  rt = sem_init(&m_start_semaphore, 0, 0);
  assert(rt == 0);

  // 创建线程，并将其主函数设为 IOThread::Main
  pthread_create(&m_thread, NULL, &IOThread::Main, this);

  // 等待初始化信号量，确保线程已经初始化完成
  sem_wait(&m_init_semaphore);

  // 输出日志，表明 I/O 线程创建成功
  DEBUGLOG("IOThread [%d] create success", m_thread_id);
}

// IOThread 类的析构函数，清理 I/O 线程及其资源
IOThread::~IOThread(){
  // 停止事件循环
  m_event_loop->stop();

  // 销毁初始化信号量和启动信号量
  sem_destroy(&m_init_semaphore);
  sem_destroy(&m_start_semaphore);

  // 等待线程结束
  pthread_join(m_thread, NULL);

  // 删除事件循环对象，释放内存
  if(m_event_loop){
    delete m_event_loop;
    m_event_loop = NULL;
  }
}

// 线程的主执行函数
void* IOThread::Main(void* arg){
  // 将传入的参数转换为 IOThread 对象指针
  IOThread* thread = static_cast<IOThread*> (arg);

  // 创建事件循环对象
  thread->m_event_loop = new EventLoop();
  // 获取并保存当前线程 ID
  thread->m_thread_id = getThreadId();

  // 通知主线程，初始化已完成
  sem_post(&thread->m_init_semaphore);

  // 输出日志，表明线程已创建并等待启动
  DEBUGLOG("IOThread %d created, wait start semaphore", thread->m_thread_id);
  // 等待启动信号量，确保线程开始执行
  sem_wait(&thread->m_start_semaphore);

  // 输出日志，表明线程已开始事件循环
  DEBUGLOG("IOThread %d start loop", thread->m_thread_id);
  // 开始事件循环
  thread->m_event_loop->loop();

  // 输出日志，表明线程已结束事件循环
  DEBUGLOG("IOThread %d end loop", thread->m_thread_id);

  return NULL;
}

// 获取 I/O 线程的事件循环对象指针
EventLoop* IOThread::getEventLoop(){
  return m_event_loop;
}

// 启动 I/O 线程
void IOThread::start(){
  // 输出日志，表明线程即将启动
  DEBUGLOG("Now invoke IOThread %d", m_thread_id);
  // 通知线程开始执行
  sem_post(&m_start_semaphore);
}

// 等待 I/O 线程执行完毕
void IOThread::join(){
  // 等待线程结束
  pthread_join(m_thread, NULL);
}

} // namespace rocket
