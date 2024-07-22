// 获取进程ID和线程ID
#ifndef ROCKET_COMMON_UTIL_H
#define ROCKET_COMMON_UTIL_H

using namespace std;

namespace rocket {

// 获取当前进程ID
pid_t getPid();

// 获取当前线程ID
pid_t getThreadId();


int64_t getNowMs();
}

#endif