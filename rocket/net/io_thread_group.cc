#include "rocket/net/io_thread_group.h"
#include "rocket/common/log.h"

namespace rocket {

// 构造函数，初始化 I/O 线程组
IOThreadGroup::IOThreadGroup(int size) : m_size(size) {
  // 调整 vector 大小以容纳指定数量的 I/O 线程
  m_io_thread_groups.resize(size);
  
  // 创建指定数量的 I/O 线程对象
  for(size_t i = 0; i < size; ++i) {
    m_io_thread_groups[i] = new IOThread();
  }
}

// 析构函数
IOThreadGroup::~IOThreadGroup() {
  // 销毁 I/O 线程对象（注意：这里应该加入删除线程对象的代码来释放内存）
  for(size_t i = 0; i < m_io_thread_groups.size(); ++i) {
    delete m_io_thread_groups[i];
  }
}

// 启动所有 I/O 线程
void IOThreadGroup::start() {
  // 遍历并启动每一个 I/O 线程
  for(size_t i = 0; i < m_io_thread_groups.size(); ++i) {
    m_io_thread_groups[i]->start();
  }
}

// 等待所有 I/O 线程完成
void IOThreadGroup::join() {
  // 遍历并等待每一个 I/O 线程完成
  for(size_t i = 0; i < m_io_thread_groups.size(); ++i) {
    m_io_thread_groups[i]->join();
  }
}

// 获取一个 I/O 线程，按轮询方式返回
IOThread* IOThreadGroup::getIOThread() {
  // 如果索引超出范围或等于 -1，重置索引为 0
  if(m_index == m_io_thread_groups.size() || m_index == -1) {
    m_index = 0;
  }

  // 返回当前索引对应的 I/O 线程，并递增索引
  return m_io_thread_groups[m_index++];
}

}
