#include <unistd.h>
#include "rocket/common/log.h"
#include "rocket/net/fd_event_group.h"
#include "rocket/net/tcp/tcp_connection.h"

namespace rocket {

  // 构造函数，初始化TCP连接
  TcpConnection::TcpConnection(IOThread* io_thread, int fd, int buffer_size, NetAddr::s_ptr peer_addr)
  : m_io_thread(io_thread), m_peer_addr(peer_addr), m_state(NotConnected), m_fd(fd) {
    // 初始化输入和输出缓冲区，缓冲区大小由参数指定
    m_in_buffer = make_shared<TcpBuffer>(buffer_size);
    m_out_buffer = make_shared<TcpBuffer>(buffer_size);

    // 获取FdEvent对象，进行非阻塞设置
    m_fd_event = FdEventGroup::getFdEventGroup()->getFdEvent(fd);
    m_fd_event->setNonBlock();

    // 设置FdEvent监听读事件，绑定回调函数onRead
    m_fd_event->listen(FdEvent::IN_EVENT, bind(&TcpConnection::onRead, this));

    // 将FdEvent对象添加到IO线程的事件循环中，以便处理I/O事件
    io_thread->getEventLoop()->addEpollEvent(m_fd_event);
  }

  // 析构函数，记录TCP连接销毁的日志
  TcpConnection::~TcpConnection() {
    DEBUGLOG("~TcpConnection");
  }

  // 处理读取事件
  void TcpConnection::onRead() {
    // 检查连接状态是否已关闭
    if (m_state != Connected) {
      ERRORLOG("onRead error, client has already disconnected, addr[%s], clientfd[%d]", 
               m_peer_addr->toString().c_str(), m_fd);
      return;
    }

    bool is_read_all = false;  // 标志是否已读取完所有数据
    bool is_close = false;     // 标志连接是否已关闭

    while (!is_read_all) {
      // 如果输入缓冲区已满，则扩大缓冲区
      if (m_in_buffer->writeAble() == 0) {
        m_in_buffer->resizeBuffer(2 * m_in_buffer->m_buffer.size());
      }

      int read_count = m_in_buffer->writeAble();  // 当前可写入的字节数
      int write_index = m_in_buffer->writeIndex();  // 写入数据的起始位置

      // 从文件描述符读取数据
      int rt = read(m_fd, &(m_in_buffer->m_buffer[write_index]), read_count);
      DEBUGLOG("success read %d bytes from addr[%s], client fd[%d]", rt, 
               m_peer_addr->toString().c_str(), m_fd);

      if (rt > 0) {
        // 数据成功读取，更新缓冲区的写入索引
        m_in_buffer->moveWriteIndex(rt);
        if (rt < read_count) {
          // 如果读取的字节数少于请求的字节数，说明数据已读取完
          is_read_all = true;
        }
      } else if (rt == 0) {
        // 对端关闭连接
        is_close = true;
      } else if (rt == -1 && errno == EAGAIN) {
        // 数据读取完毕，文件描述符暂时无数据
        is_read_all = true;
      }
    }

    // 如果对端关闭连接，处理清理工作
    if (is_close) {
      INFOLOG("peer closed, peer addr [%s], clientfd [%d]", 
              m_peer_addr->toString().c_str(), m_fd);
      clear();
      return;
    }

    // 如果未读取完所有数据，记录错误日志
    if (!is_read_all) {
      ERRORLOG("not read all data");
    }

    // 执行进一步处理
    excute();
  }

  // 执行处理逻辑，将输入数据存储到输出缓冲区
  void TcpConnection::excute() {
    vector<char> tmp;
    int size = m_in_buffer->readAble();  // 输入缓冲区中可读的字节数
    tmp.resize(size);
    m_in_buffer->readFromBuffer(tmp, size);  // 从输入缓冲区读取数据

    string msg(tmp.begin(), tmp.end());  // 将读取的数据转换为字符串

    INFOLOG("success get request from client[%s]", m_peer_addr->toString().c_str());

    // 将消息写入输出缓冲区
    m_out_buffer->writeToBuffer(msg.c_str(), msg.length());

    // 设置写事件监听回调函数
    m_fd_event->listen(FdEvent::OUT_EVENT, bind(&TcpConnection::onWrite, this));
    m_io_thread->getEventLoop()->addEpollEvent(m_fd_event);  // 更新事件循环
  }

  // 处理写入事件，将数据写入文件描述符
  void TcpConnection::onWrite() {
    // 检查连接状态是否已关闭
    if (m_state != Connected) {
      ERRORLOG("onWrite error, client has already disconnected, addr[%s], clientfd[%d]", 
               m_peer_addr->toString().c_str(), m_fd);
      return;
    }

    bool is_write_all = false;  // 标志是否已写入所有数据

    while (true) {
      // 如果输出缓冲区没有数据要写入，记录日志并退出
      if (m_out_buffer->readAble() == 0) {
        DEBUGLOG("no data need to send to client [%s]", m_peer_addr->toString().c_str());
        is_write_all = true;
        break;
      }

      int write_size = m_out_buffer->readAble();  // 当前可写入的字节数
      int read_index = m_out_buffer->readIndex();  // 读取数据的起始位置

      // 向文件描述符写入数据
      int rt = write(m_fd, &(m_out_buffer->m_buffer[read_index]), write_size);

      if (rt >= write_size) {
        // 数据成功写入
        DEBUGLOG("success write %d bytes to client [%s]", rt, 
                 m_peer_addr->toString().c_str());
        is_write_all = true;
        break;
      }

      if (rt == -1 && errno == EAGAIN) {
        // 写入数据时发生EAGAIN错误，说明写入缓冲区已满
        ERRORLOG("write data error, errno == EAGAIN and rt == -1");
        break;
      }
    }

    // 如果所有数据都已写入，取消写事件的监听
    if (is_write_all) {
      m_fd_event->cancel(FdEvent::OUT_EVENT);
      m_io_thread->getEventLoop()->addEpollEvent(m_fd_event);
    }
  }

  // 设置TCP连接的状态
  void TcpConnection::setState(const TcpState state) {
    m_state = state;
  }

  // 获取TCP连接的状态
  TcpState TcpConnection::getState() {
    return m_state;
  }

  // 清理TCP连接，关闭文件描述符并更新状态
  void TcpConnection::clear() {
    if (m_state == Closed) {
      return;  // 如果已关闭，无需进一步处理
    }

    m_fd_event->cancel(FdEvent::IN_EVENT);
    m_fd_event->cancel(FdEvent::OUT_EVENT);

    // 从事件循环中删除FdEvent对象
    m_io_thread->getEventLoop()->deleteEpollEvent(m_fd_event);

    m_state = Closed;  // 更新状态为已关闭
  }

  // 服务器主动关闭连接，设置状态为半关闭并关闭文件描述符
  void TcpConnection::shutdown() {
    if (m_state == Closed || m_state == NotConnected) {
      return;  // 如果连接已关闭或未连接，无需进一步处理
    }

    m_state = HalfClosing;  // 设置状态为半关闭

    // 关闭文件描述符的读写操作
    ::shutdown(m_fd, SHUT_RDWR);
  }

}
