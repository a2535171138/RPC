#include "rocket/net/tcp/tcp_server.h"  // 引入TcpServer头文件
#include "rocket/net/eventloop.h"  // 引入EventLoop头文件
#include "rocket/common/log.h"  // 引入日志头文件
#include "rocket/net/tcp/tcp_connection.h"

namespace rocket {  // 定义命名空间rocket

TcpServer::TcpServer(NetAddr::s_ptr local_addr) : m_local_addr(local_addr) {  // 构造函数，初始化本地地址
  init();  // 调用初始化函数
  INFOLOG("rocket TcpServer listen success on [%s]", m_local_addr->toString().c_str());  // 记录服务器监听成功的日志
}

TcpServer::~TcpServer() {  // 析构函数，释放资源
  if (m_main_event_loop) {  // 如果主事件循环指针不为空
    delete m_main_event_loop;  // 删除主事件循环指针
    m_main_event_loop = NULL;  // 将主事件循环指针置为空
  }
}

void TcpServer::init() {  // 初始化函数
  m_acceptor = make_shared<TcpAcceptor>(m_local_addr);  // 创建TcpAcceptor对象

  m_main_event_loop = EventLoop::GetCurrentEventLoop();  // 获取当前事件循环

  m_io_thread_group = new IOThreadGroup(2);  // 创建IO线程组，线程数量为2

  m_listen_fd_event = new FdEvent(m_acceptor->getListnFd());  // 创建监听文件描述符事件
  m_listen_fd_event->listen(FdEvent::IN_EVENT, bind(&TcpServer::onAccept, this));  // 监听文件描述符事件，并绑定接受连接的回调函数

  m_main_event_loop->addEpollEvent(m_listen_fd_event);  // 将监听文件描述符事件添加到主事件循环中
}

void TcpServer::onAccept() {  // 处理接受连接的函数
  auto re = m_acceptor->accept();
  int client_fd = re.first;
  NetAddr::s_ptr peer_addr = re.second;

  m_client_counts++;  // 增加客户端连接计数

  IOThread* io_thread = m_io_thread_group->getIOThread();
  TcpConnection::s_ptr connection = make_shared<TcpConnection>(io_thread, client_fd, 128, peer_addr);

  connection->setState(Connected);
  m_client.insert(connection);

  INFOLOG("TcpServer succ get client, fd=%d", client_fd);  // 记录成功接受客户端连接的日志
}

void TcpServer::start() {  // 启动服务器
  m_io_thread_group->start();  // 启动IO线程组
  m_main_event_loop->loop();  // 启动主事件循环
}

}
