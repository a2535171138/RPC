#include <sys/socket.h>          // 提供 socket 函数和相关宏
#include <unistd.h>             // 提供 close 函数
#include <string.h>            // 提供 strerror 函数
#include "rocket/common/log.h"  // 提供日志记录功能
#include "rocket/net/tcp/tcp_client.h"  // 提供 TcpClient 类定义
#include "rocket/net/eventloop.h"       // 提供事件循环功能
#include "rocket/net/fd_event_group.h"  // 提供文件描述符事件组功能

namespace rocket {

// 构造函数，初始化 TcpClient 对象
TcpClient::TcpClient(NetAddr::s_ptr peer_addr) : m_peer_addr(peer_addr) {
    // 获取当前事件循环对象
    m_event_loop = EventLoop::GetCurrentEventLoop();
    
    // 创建一个 TCP socket
    m_fd = socket(peer_addr->getFamily(), SOCK_STREAM, 0);
    if (m_fd < 0) {
        ERRORLOG("TcpClient::TcpClient() error, failed to create fd");
        return;
    }

    // 获取文件描述符事件对象，并设置为非阻塞模式
    m_fd_event = FdEventGroup::getFdEventGroup()->getFdEvent(m_fd);
    m_fd_event->setNonBlock();

    // 创建一个 TcpConnection 对象，并初始化
    m_connection = std::make_shared<TcpConnection>(m_event_loop, m_fd, 128, peer_addr, nullptr, TcpConnectionByClient);
    m_connection->setConnectionType(TcpConnectionByClient);
}

// 析构函数，关闭文件描述符
TcpClient::~TcpClient() {
    if (m_fd > 0) {
        close(m_fd);
    }
}

// 异步连接函数
// 如果连接成功，将执行 done 回调函数
void TcpClient::connect(std::function<void()> done) {
    int rt = ::connect(m_fd, m_peer_addr->getSockAddr(), m_peer_addr->getSockLen());
    if (rt == 0) {
        // 连接成功
        DEBUGLOG("connect [%s] success", m_peer_addr->toString().c_str());
        if (done) {
            done();
        }
    } else if (rt == -1) {
        if (errno == EINPROGRESS) {
            // 连接正在进行，监听可写事件以判断连接是否成功
            m_fd_event->listen(FdEvent::OUT_EVENT, [this, done]() {
                int error = 0;
                socklen_t error_len = sizeof(error);
                // 获取连接错误信息
                getsockopt(m_fd, SOL_SOCKET, SO_ERROR, &error, &error_len);
                bool is_connected_succ = false;
                if (error == 0) {
                    // 连接成功
                    DEBUGLOG("connect [%s] success", m_peer_addr->toString().c_str());
                    is_connected_succ = true;
                    m_connection->setState(Connected);
                } else {
                    // 连接失败
                    ERRORLOG("connect error, errno=%d, error=%s", errno, strerror(errno));
                }

                // 连接完成后取消可写事件的监听
                m_fd_event->cancel(FdEvent::OUT_EVENT);
                m_event_loop->addEpollEvent(m_fd_event);

                // 如果连接成功，执行回调函数
                if (is_connected_succ && done) {
                    done();
                }
            });

            // 添加到事件循环中
            m_event_loop->addEpollEvent(m_fd_event);
            if (!m_event_loop->isLooping()) {
                m_event_loop->loop();
            }
        } else {
            // 连接失败
            ERRORLOG("connect error, errno=%d, error=%s", errno, strerror(errno));
        }
    }
}

// 异步发送消息
// 如果发送成功，将调用 done 函数，传递消息对象
void TcpClient::writeMessage(AbstractProtocol::s_ptr message, std::function<void(AbstractProtocol::s_ptr)> done) {
    // 将消息对象推送到 TcpConnection 的发送队列
    // 启动连接的写事件监听
    m_connection->pushSendMessage(message, done);
    m_connection->listenWrite();
}

// 异步读取消息
// 如果读取成功，将调用 done 函数，传递消息对象
void TcpClient::readMessage(const std::string& msg_id, std::function<void(AbstractProtocol::s_ptr)> done) {
    // 将读取请求推送到 TcpConnection 的读取队列
    // 启动连接的读事件监听
    m_connection->pushReadMessage(msg_id, done);
    m_connection->listenRead();
}

}
