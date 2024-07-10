// 获取进程ID和线程ID
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include "rocket/common/util.h"


namespace rocket {

// 缓存的全局进程ID
static int g_pid = 0;
// 线程局部存储的线程ID
static thread_local int get_thread_id = 0;

// 获取当前进程ID
pid_t getPid(){
  // 返回缓存的进程ID
  if(g_pid != 0){
    return g_pid;
  }

  // 调用系统函数获取当前进程ID
  return getpid();
}

// 获取当前线程ID
pid_t getThreadId(){
  // 返回缓存的线程ID
  if (get_thread_id != 0){
    return get_thread_id;
  }

  // 调用系统调用获取当前线程ID
  return syscall(SYS_gettid);
}

}
