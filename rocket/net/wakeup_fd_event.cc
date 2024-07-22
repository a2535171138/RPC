#include <unistd.h>
#include "rocket/net/wakeup_fd_event.h"
#include "rocket/common/log.h"

namespace rocket {

// 构造函数，调用基类FdEvent的构造函数进行初始化
WakeUpFdEvent::WakeUpFdEvent(int fd) : FdEvent(fd) {
}

// 析构函数，目前没有特殊操作
WakeUpFdEvent::~WakeUpFdEvent() {
}

// 唤醒函数
void WakeUpFdEvent::wakeup() {
  char buf[8] = {'a'};  // 创建一个8字节的缓冲区，并填充字符'a'

  // 向文件描述符写入8字节的数据
  int rt = write(m_fd, buf, 8);

  // 检查实际写入的字节数是否等于8
  if (rt != 8) {
    ERRORLOG("write to wakeup fd less than 8 bytes, fd[%d]", m_fd);
  } else {
    DEBUGLOG("success read 8 bytes");
  }
}

}
