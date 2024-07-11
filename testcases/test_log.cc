#include <pthread.h>
#include "rocket/common/log.h"
#include "rocket/common/config.h"

// 线程函数
void* fun(void*){
  int i = 20;
  while(i--){
    // 使用 DEBUGLOG 宏记录调试信息
    DEBUGLOG("debug this is thread in %s", "fun");
    INFOLOG("info this is thread in %s", "fun");
  }
  return NULL;
}

int main(){
  rocket::Config::SetGlobalConfig("../conf/rocket.xml");  // 设置全局配置文件路径

  rocket::Logger::InitGlobalLogger();  // 初始化全局日志记录器
  
  // 创建线程，调用 fun 函数作为线程入口
  pthread_t thread;
  pthread_create(&thread, NULL, &fun, NULL);

  int i = 20;
  while(i--){
    // 主线程中使用 DEBUGLOG 宏记录调试信息
    DEBUGLOG("test debug log %s", "11");
    INFOLOG("test info log %s", "11");
  }

  // 等待线程执行完毕
  pthread_join(thread, NULL);
  
  return 0;
}
