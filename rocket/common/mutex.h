// 互斥锁
#ifndef ROCKET_COMMON_MUTEX_H
#define ROCKET_COMMON_MUTEX_H

#include <pthread.h>  // POSIX线程库

namespace rocket{

// 互斥锁模板类
template <class T>
class ScopeMutex{
  public:
    // 构造函数，加锁
    ScopeMutex(T& mutex) : m_mutex(mutex){
      m_mutex.lock(); // 加锁
      m_is_lock = true; // 记录锁状态
    }
    // 析构函数，解锁
    ~ScopeMutex(){
      m_mutex.unlock(); // 解锁
      m_is_lock = false;  // 更新锁状态
    }

    // 手动加锁
    void lock(){
      if(!m_is_lock){
        m_mutex.lock(); // 加锁
      }
    }

    void unlock(){
      if(m_is_lock){
        m_mutex.unlock(); // 解锁
      }
    }

  private:
    T& m_mutex; // 互斥锁对象引用
    bool m_is_lock{false};  // 锁状态标志
};

// 互斥锁类
class Mutex{
 public:
  Mutex(){
    pthread_mutex_init(&m_mutex, NULL);  // 初始化互斥锁
  }

  ~Mutex(){
    pthread_mutex_destroy(&m_mutex);  // 销毁互斥锁
  }

  void lock(){
    pthread_mutex_lock(&m_mutex);  // 加锁
  }

  void unlock(){
    pthread_mutex_unlock(&m_mutex);  // 解锁
  }

  pthread_mutex_t* getMutex(){
    return &m_mutex;
  }

 private:
  pthread_mutex_t m_mutex;  // POSIX线程互斥锁对象

};


}
#endif