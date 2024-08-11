#ifndef ROCKET_COMMON_UTIL_H
#define ROCKET_COMMON_UTIL_H

using namespace std;

namespace rocket {

// 获取当前进程ID
// 返回值：当前进程的 ID
pid_t getPid();

// 获取当前线程ID
// 返回值：当前线程的 ID
pid_t getThreadId();

// 获取当前时间的毫秒数
// 返回值：从 Unix 纪元（1970-01-01 00:00:00 UTC）以来经过的毫秒数
int64_t getNowMs();

// 将网络字节序的 4 字节数据转换为主机字节序
// 参数：buf - 指向网络字节序数据的指针
// 返回值：转换后的主机字节序的 32 位整数
int32_t getInt32FromNetByte(const char* buf);

}

#endif
