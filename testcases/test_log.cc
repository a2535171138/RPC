#include <pthread.h>
#include "rocket/common/log.h"

// 线程函数
void* fun(void*){
  // 使用 DEBUGLOG 宏记录调试信息
  DEBUGLOG("this is thread in %s", "fun");
  return NULL;
}

int main(){
  pthread_t thread;
  // 创建线程，调用 fun 函数作为线程入口
  int ret = pthread_create(&thread, NULL, &fun, NULL);
  if (ret != 0) {
      perror("pthread_create failed");
  }

  // 主线程中使用 DEBUGLOG 宏记录调试信息
  DEBUGLOG("test log %s", "11");

  // 等待线程执行完毕
  pthread_join(thread, NULL);
  return 0;
}
