#include "rocket/common/run_time.h"

namespace rocket{

// 使用 thread_local 关键字声明一个线程局部的运行时指针，确保每个线程都有自己独立的 RunTime 实例
thread_local RunTime* t_run_time = NULL;

// 获取当前线程的运行时实例
RunTime* RunTime::GetRunTime(){
  if(t_run_time){  // 如果当前线程已经有一个运行时实例，直接返回
    return t_run_time;
  }
  t_run_time = new RunTime();  // 否则，创建一个新的运行时实例，并将其赋值给 t_run_time
  return t_run_time;
}

}
