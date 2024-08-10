#include "rocket/net/tcp/tcp_server.h"  // 引入 TcpServer 头文件
#include "rocket/net/eventloop.h"  // 引入 EventLoop 头文件
#include "rocket/common/log.h"  // 引入日志头文件
#include "rocket/net/tcp/tcp_connection.h"  // 引入 TcpConnection 头文件

namespace rocket {  // 定义命名空间 rocket

// 构造函数，初始化 TcpServer 对象
TcpServer::TcpServer(NetAddr::s_ptr local_addr) : m_local_addr(local_addr) {
    init();  // 调用初始化函数
    INFOLOG("rocket TcpServer listen success on [%s]", m_local_addr->toString().c_str());  // 记录服务器监听成功的日志
}

// 析构函数，释放资源
TcpServer::~TcpServer() {
    if (m_main_event_loop) {  // 如果主事件循环指针不为空
        delete m_main_event_loop;  // 删除主事件循环指针
        m_main_event_loop = NULL;  // 将主事件循环指针置为空
    }
}

// 初始化函数，设置服务器的基本参数和事件
void TcpServer::init() {
    m_acceptor = make_shared<TcpAcceptor>(m_local_addr);  // 创建 TcpAcceptor 对象，负责接受连接

    m_main_event_loop = EventLoop::GetCurrentEventLoop();  // 获取当前线程的事件循环对象

    m_io_thread_group = new IOThreadGroup(2);  // 创建 IO 线程组，包含两个线程

    m_listen_fd_event = new FdEvent(m_acceptor->getListnFd());  // 创建监听文件描述符事件
    m_listen_fd_event->listen(FdEvent::IN_EVENT, bind(&TcpServer::onAccept, this));  // 监听文件描述符的读事件，并绑定接受连接的回调函数

    m_main_event_loop->addEpollEvent(m_listen_fd_event);  // 将监听文件描述符事件添加到主事件循环中
}

// 处理接收到的连接请求
void TcpServer::onAccept() {
    auto re = m_acceptor->accept();  // 接受一个连接
    int client_fd = re.first;  // 客户端文件描述符
    NetAddr::s_ptr peer_addr = re.second;  // 客户端地址

    m_client_counts++;  // 增加客户端连接计数

    IOThread* io_thread = m_io_thread_group->getIOThread();  // 从 IO 线程组中获取一个 IO 线程
    TcpConnection::s_ptr connection = make_shared<TcpConnection>(io_thread->getEventLoop(), client_fd, 128, peer_addr);  // 创建 TcpConnection 对象

    connection->setState(Connected);  // 设置连接状态为已连接
    m_client.insert(connection);  // 将连接添加到连接集合中

    INFOLOG("TcpServer succ get client, fd=%d", client_fd);  // 记录成功接受客户端连接的日志
}

// 启动服务器，开始接收和处理连接
void TcpServer::start() {
    m_io_thread_group->start();  // 启动 IO 线程组
    m_main_event_loop->loop();  // 启动主事件循环，开始处理事件
}

}
