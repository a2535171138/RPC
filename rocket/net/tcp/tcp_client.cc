#include <sys/socket.h>          // 提供 socket 函数和相关宏
#include <unistd.h>              // 提供 close 函数
#include <string.h>              // 提供 strerror 函数
#include "rocket/common/log.h"   // 提供日志记录功能
#include "rocket/net/tcp/tcp_client.h"  // 提供 TcpClient 类定义
#include "rocket/net/eventloop.h"       // 提供事件循环功能
#include "rocket/net/fd_event_group.h"  // 提供文件描述符事件组功能
#include "rocket/common/error_code.h"   // 提供错误代码定义
#include "rocket/net/tcp/net_addr.h"    // 提供网络地址定义

namespace rocket {

// 构造函数，初始化 TcpClient 对象
// 参数: peer_addr - 目标服务器的网络地址
TcpClient::TcpClient(NetAddr::s_ptr peer_addr) : m_peer_addr(peer_addr) {
    // 获取当前线程的事件循环对象
    m_event_loop = EventLoop::GetCurrentEventLoop();
    
    // 创建一个 TCP socket，指定地址族和流式套接字
    m_fd = socket(peer_addr->getFamily(), SOCK_STREAM, 0);
    if (m_fd < 0) {
        ERRORLOG("TcpClient::TcpClient() error, failed to create fd");
        return;
    }

    // 获取文件描述符事件对象，并设置为非阻塞模式
    m_fd_event = FdEventGroup::getFdEventGroup()->getFdEvent(m_fd);
    m_fd_event->setNonBlock();

    // 创建一个 TcpConnection 对象，初始化连接
    m_connection = std::make_shared<TcpConnection>(m_event_loop, m_fd, 128, peer_addr, nullptr, TcpConnectionByClient);
    m_connection->setConnectionType(TcpConnectionByClient);
}

// 析构函数，释放资源，关闭文件描述符
TcpClient::~TcpClient() {
    if (m_fd > 0) {
        close(m_fd);
    }
}

// 异步连接函数
// 如果连接成功，将执行 done 回调函数
void TcpClient::connect(std::function<void()> done) {
    // 发起连接请求
    int rt = ::connect(m_fd, m_peer_addr->getSockAddr(), m_peer_addr->getSockLen());
    if (rt == 0) {
        // 连接成功
        DEBUGLOG("connect [%s] success", m_peer_addr->toString().c_str());
        m_connection->setState(Connected);
        initLocalAddr();  // 初始化本地地址
        if (done) {
            done();  // 调用回调函数
        }
    } else if (rt == -1) {
        if (errno == EINPROGRESS) {
            // 连接正在进行中，设置监听可写事件以判断连接是否成功
            m_fd_event->listen(FdEvent::OUT_EVENT, 
            [this, done]() {
                // 再次尝试连接以确认连接状态
                int rt = ::connect(m_fd, m_peer_addr->getSockAddr(), m_peer_addr->getSockLen());
                if((rt < 0 && errno == EISCONN) || (rt == 0)){
                    DEBUGLOG("connect [%s] success", m_peer_addr->toString().c_str());
                    initLocalAddr();
                    m_connection->setState(Connected);
                }
                else {
                    if(errno == ECONNREFUSED){
                        m_connect_error_code = ERROR_PEER_CLOSED;  // 连接被拒绝
                        m_connect_error_info = "connect refused, sys error = " + std::string(strerror(errno));
                    }
                    else {
                        m_connect_error_code = ERROR_FAILED_CONNECT;  // 未知错误导致连接失败
                        m_connect_error_info = "connect unknown error, sys error = " + std::string(strerror(errno));
                    }
                    ERRORLOG("connect error, errno = %d, error = %s", errno, strerror(errno));
                    close(m_fd);
                    m_fd = socket(m_peer_addr->getFamily(), SOCK_STREAM, 0);  // 重新创建 socket
                }
                
                m_event_loop->deleteEpollEvent(m_fd_event);
                DEBUGLOG("now begin to done");

                if (done) {
                    done();
                }
            }, 
            [this, done](){
                // 连接失败的错误处理
                if(errno == ECONNREFUSED){
                    m_connect_error_code = ERROR_FAILED_CONNECT;
                    m_connect_error_info = "connect refused, sys error = " + std::string(strerror(errno));
                }
                else {
                    m_connect_error_code = ERROR_FAILED_CONNECT;
                    m_connect_error_info = "connect unknown error, sys error = " + std::string(strerror(errno));
                }
                ERRORLOG("connect error, errno=%d, error=%s", errno, strerror(errno));
            });

            // 添加文件描述符事件到事件循环中
            m_event_loop->addEpollEvent(m_fd_event);
            if (!m_event_loop->isLooping()) {
                m_event_loop->loop();  // 启动事件循环
            }
        } else {
            // 立即连接失败的错误处理
            m_connect_error_code = ERROR_FAILED_CONNECT;
            m_connect_error_info = "connect error, sys error = " + std::string(strerror(errno));
            ERRORLOG("connect error, errno=%d, error=%s", errno, strerror(errno));
            if (done) {
                done();
            }
        }
    }
}

// 停止客户端操作
void TcpClient::stop(){
    if(m_event_loop->isLooping()){
        m_event_loop->stop();  // 停止事件循环
    }
}

// 异步发送消息
// 如果发送成功，将调用 done 回调函数，传递消息对象
void TcpClient::writeMessage(AbstractProtocol::s_ptr message, std::function<void(AbstractProtocol::s_ptr)> done) {
    // 将消息推送到 TcpConnection 的发送队列，并启动写事件监听
    m_connection->pushSendMessage(message, done);
    m_connection->listenWrite();
}

// 异步读取消息
// 如果读取成功，将调用 done 回调函数，传递消息对象
void TcpClient::readMessage(const std::string& msg_id, std::function<void(AbstractProtocol::s_ptr)> done) {
    // 将读取请求推送到 TcpConnection 的读取队列，并启动读事件监听
    m_connection->pushReadMessage(msg_id, done);
    m_connection->listenRead();
}

// 获取连接错误码
int TcpClient::getConnectErrorCode(){
    return m_connect_error_code;
}

// 获取连接错误信息
std::string TcpClient::getConnectErrorInfo(){
    return m_connect_error_info;
}

// 获取对端（服务器）地址
NetAddr::s_ptr TcpClient::getPeerAddr(){
    return m_peer_addr;
}

// 获取本地（客户端）地址
NetAddr::s_ptr TcpClient::getLocalAddr(){
    return m_local_addr;
}

// 初始化本地地址信息
void TcpClient::initLocalAddr(){
    sockaddr_in local_addr;
    socklen_t len = sizeof(local_addr);

    // 获取本地 socket 地址
    int ret = getsockname(m_fd, reinterpret_cast<sockaddr*>(&local_addr), &len);
    if(ret != 0){
        ERRORLOG("initLocalAddr error, getsockname error, errno = %d, error = %s", errno, strerror(errno));
        return;
    }

    // 创建本地地址对象并保存
    m_local_addr = std::make_shared<IPNetAddr>(local_addr);
}

// 添加定时事件
void TcpClient::addTimerEvent(TimerEvent::s_ptr timer_event){
    m_event_loop->addTimerEvent(timer_event);  // 将定时事件添加到事件循环中
}

}  // namespace rocket
