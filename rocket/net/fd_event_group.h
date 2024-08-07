#ifndef ROCKET_NET_FD_EVENT_GROUP_H
#define ROCKET_NET_FD_EVENT_GROUP_H

#include <vector>
#include "rocket/common/mutex.h"
#include "rocket/net/fd_event.h"

namespace rocket {

  // FdEventGroup类用于管理多个FdEvent对象
  class FdEventGroup {
    public:
      // 构造函数，初始化FdEventGroup对象，指定组大小
      FdEventGroup(int size);

      // 析构函数，清理FdEventGroup对象
      ~FdEventGroup();

      // 获取特定文件描述符对应的FdEvent对象
      FdEvent* getFdEvent(int fd);

      // 获取FdEventGroup的单例实例
      static FdEventGroup* getFdEventGroup();

    private:
      int m_size {0};                  // FdEventGroup的大小
      vector<FdEvent*> m_fd_group;     // 存储FdEvent对象的向量
      Mutex m_mutex;                   // 用于同步访问的互斥锁
  };

}

#endif
