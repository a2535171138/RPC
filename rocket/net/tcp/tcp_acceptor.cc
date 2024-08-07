#include <assert.h>  // 引入标准库中的assert头文件
#include <sys/socket.h>  // 引入用于套接字编程的头文件
#include <fcntl.h>  // 引入用于文件控制的头文件
#include <string.h>  // 引入标准库中的string.h头文件
#include "rocket/common/log.h"  // 引入日志头文件
#include "rocket/net/tcp/net_addr.h"  // 引入NetAddr头文件
#include "rocket/net/tcp/tcp_acceptor.h"  // 引入TcpAcceptor头文件

namespace rocket {  // 定义命名空间rocket

TcpAcceptor::TcpAcceptor(NetAddr::s_ptr local_addr) : m_local_addr(local_addr) {  // 构造函数，初始化本地地址
  if (!local_addr->checkValid()) {  // 检查本地地址是否有效
    ERRORLOG("invalid local addr %s", local_addr->toString().c_str());  // 记录错误日志
    exit(0);  // 退出程序
  }

  m_family = m_local_addr->getFamily();  // 获取地址族

  m_listenfd = socket(m_family, SOCK_STREAM, 0);  // 创建套接字

  if (m_listenfd < 0) {  // 如果套接字创建失败
    ERRORLOG("invalid listenfd %d", m_listenfd);  // 记录错误日志
    exit(0);  // 退出程序
  }

  int val = 1;
  if (setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) != 0) {  // 设置套接字选项，使地址可以重复使用
    ERRORLOG("setsockopt REUSEADDR error, errno=%d, error=%s", errno, strerror(errno));  // 记录错误日志
  }

  socklen_t len = m_local_addr->getSockLen();  // 获取本地地址的长度
  if (bind(m_listenfd, m_local_addr->getSockAddr(), len) != 0) {  // 绑定套接字到本地地址
    ERRORLOG("bind error, errno=%d, error=%s", errno, strerror(errno));  // 记录错误日志
    exit(0);  // 退出程序
  }

  if (listen(m_listenfd, 1000) != 0) {  // 监听套接字，最大连接数为1000
    ERRORLOG("listen error, errno=%d, error=%s", errno, strerror(errno));  // 记录错误日志
    exit(0);  // 退出程序
  }
}

TcpAcceptor::~TcpAcceptor() {  // 析构函数

}

int TcpAcceptor::getListnFd() {  // 获取监听文件描述符
  return m_listenfd;
}

pair<int, NetAddr::s_ptr> TcpAcceptor::accept() {  // 接受连接
  if (m_family == AF_INET) {  // 如果地址族是IPv4
    sockaddr_in client_addr;  // 定义客户端地址
    memset(&client_addr, 0, sizeof(client_addr));  // 将客户端地址结构体清零
    socklen_t client_addr_len = sizeof(client_addr_len);  // 获取客户端地址的长度

    int client_fd = ::accept(m_listenfd, reinterpret_cast<sockaddr*>(&client_addr), &client_addr_len);  // 接受连接
    if (client_fd < 0) {  // 如果连接失败
      ERRORLOG("accept error, errno=%d, error=%s", errno, strerror(errno));  // 记录错误日志
    }

    IPNetAddr::s_ptr peer_addr = make_shared<IPNetAddr>(client_addr);  // 将客户端地址转换为IPNetAddr对象
    INFOLOG("A client have accepted succ, peer addr [%s]", peer_addr->toString().c_str());  // 记录连接成功日志

    return make_pair(client_fd, peer_addr) ;  // 返回客户端文件描述符
  } else {  // 如果地址族不是IPv4
    return std::make_pair(-1, nullptr);  // 返回错误码
  }
}

}
