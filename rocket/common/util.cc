// 获取进程ID和线程ID
#include <sys/types.h>      // 提供基本的系统数据类型
#include <unistd.h>         // 提供获取进程ID的函数
#include <sys/syscall.h>    // 提供系统调用的常量
#include <sys/time.h>       // 提供获取当前时间的函数
#include <arpa/inet.h>      // 提供网络字节序转换函数
#include <cstring>          // 提供内存操作函数
#include "rocket/common/util.h"  // 引入自定义的头文件

namespace rocket {

// 缓存的全局进程ID
static int g_pid = 0;
// 线程局部存储的线程ID
static thread_local int get_thread_id = 0;

// 获取当前进程ID
pid_t getPid() {
  // 如果缓存中已有进程ID，则直接返回
  if (g_pid != 0) {
    return g_pid;
  }

  // 调用系统函数获取当前进程ID
  return getpid();
}

// 获取当前线程ID
pid_t getThreadId() {
  // 如果线程局部存储中已有线程ID，则直接返回
  if (get_thread_id != 0) {
    return get_thread_id;
  }

  // 调用系统调用获取当前线程ID
  return syscall(SYS_gettid);
}

// 获取当前时间的毫秒数
int64_t getNowMs() {
  timeval val;
  // 获取当前时间
  gettimeofday(&val, NULL);
  // 将秒数转换为毫秒数，并加上微秒数的毫秒部分
  return val.tv_sec * 1000 + val.tv_usec / 1000;
}

// 将网络字节序的 4 字节数据转换为主机字节序
int32_t getInt32FromNetByte(const char* buf) {
  int32_t re;
  // 从缓冲区中拷贝 4 字节数据到整型变量
  memcpy(&re, buf, sizeof(re));
  // 将网络字节序转换为主机字节序
  return ntohl(re);
}

}
