#include "rocket/net/fd_event_group.h"
#include "rocket/common/mutex.h"
#include "rocket/common/log.h"

namespace rocket {

// 全局静态变量，存储FdEventGroup的单例实例
static FdEventGroup* g_fd_event_group = NULL;

// 获取FdEventGroup的单例实例
FdEventGroup* FdEventGroup::getFdEventGroup() {
  if (g_fd_event_group != NULL) {
    return g_fd_event_group;
  }

  // 如果实例尚未创建，创建一个新的FdEventGroup实例，大小为128
  g_fd_event_group = new FdEventGroup(128);
  return g_fd_event_group;
}

// FdEventGroup构造函数，初始化指定大小的FdEvent对象组
FdEventGroup::FdEventGroup(int size) : m_size(size) {
  for (int i = 0; i < m_size; ++i) {
    // 创建新的FdEvent对象并添加到组中
    m_fd_group.push_back(new FdEvent(i));
  }
}

// FdEventGroup析构函数，清理FdEvent对象组中的所有对象
FdEventGroup::~FdEventGroup() {
  for (int i = 0; i < m_size; ++i) {
    if (m_fd_group[i] != NULL) {
      delete m_fd_group[i];
      m_fd_group[i] = NULL;
    }
  }
}

// 获取特定文件描述符对应的FdEvent对象
FdEvent* FdEventGroup::getFdEvent(int fd) {
  // 使用互斥锁确保线程安全
  ScopeMutex<Mutex> lock(m_mutex);
  if ((size_t)fd < m_fd_group.size()) {
    return m_fd_group[fd];
  }

  // 如果文件描述符超出当前组大小，扩展组大小
  int new_size = int(fd * 1.5);
  for (int i = m_fd_group.size(); i < new_size; ++i) {
    m_fd_group.push_back(new FdEvent(i));
  }

  return m_fd_group[fd];
}

}
