#ifndef ROCKET_NET_IO_THREAD_GROUP_H
#define ROCKET_NET_IO_THREAD_GROUP_H

#include <vector>
#include "rocket/common/log.h"
#include "rocket/net/io_thread.h"

using namespace std;

namespace rocket{

// IOThreadGroup 类声明，管理一组 I/O 线程
class IOThreadGroup {
  public:
    // 构造函数，传入要创建的 I/O 线程数量
    IOThreadGroup(int size);

    // 析构函数，释放资源
    ~IOThreadGroup();

    // 启动所有 I/O 线程
    void start();

    // 等待所有 I/O 线程完成
    void join();

    // 获取一个 I/O 线程，按轮询方式返回
    IOThread* getIOThread();

  private:
    int m_size {0}; // I/O 线程的数量

    vector<IOThread*> m_io_thread_groups; // 存储所有 I/O 线程的指针

    int m_index {0}; // 当前轮询到的线程索引
};

}

#endif
