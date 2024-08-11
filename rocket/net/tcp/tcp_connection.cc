#include <unistd.h>                 // 提供对 POSIX 操作系统 API 的访问
#include <functional>              // 提供函数对象和其他函数相关功能
#include "rocket/common/log.h"     // 提供日志记录功能
#include "rocket/net/fd_event_group.h"  // 提供文件描述符事件组功能
#include "rocket/net/tcp/tcp_connection.h"  // 提供 TCP 连接类
#include "rocket/net/coder/string_coder.h"  // 提供字符串编码器
#include "rocket/net/coder/tinypb_coder.h"  // 提供 TinyPB 编码器

namespace rocket {

// 构造函数，初始化 TCP 连接
TcpConnection::TcpConnection(EventLoop* event_loop, int fd, int buffer_size, NetAddr::s_ptr peer_addr, TcpConnectionType type)
: m_event_loop(event_loop), m_peer_addr(peer_addr), m_state(NotConnected), m_fd(fd), m_connection_type(type) {
    // 初始化输入和输出缓冲区，缓冲区大小由参数指定
    m_in_buffer = make_shared<TcpBuffer>(buffer_size);
    m_out_buffer = make_shared<TcpBuffer>(buffer_size);

    // 获取 FdEvent 对象，并设置为非阻塞模式
    m_fd_event = FdEventGroup::getFdEventGroup()->getFdEvent(fd);
    m_fd_event->setNonBlock();

    // 初始化编码器
    m_coder = new TinyPBCoder();

    // 如果是服务器端连接，则启动读事件监听
    if (m_connection_type == TcpConnectionByServer) {
        listenRead();
    }
}

// 析构函数，记录 TCP 连接销毁的日志
TcpConnection::~TcpConnection() {
    DEBUGLOG("~TcpConnection");
    if (m_coder) {
        delete m_coder;  // 释放编码器的内存
        m_coder = NULL;
    }
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
    if (m_connection_type == TcpConnectionByServer) {
        vector<AbstractProtocol::s_ptr> result;
        vector<AbstractProtocol::s_ptr> replay_messages;
        m_coder->decode(result, m_in_buffer);  // 解码输入数据
        for(size_t i = 0; i < result.size(); ++i){
            INFOLOG("success get request[%s] from client[%s]", result[i]->m_req_id.c_str(), m_peer_addr->toString().c_str());

            // 创建回应消息
            shared_ptr<TinyPBProtocol> message = make_shared<TinyPBProtocol>();
            message->m_pb_data = "hello, this is rocket rpc test data";
            message->m_req_id = result[i]->m_req_id;
            replay_messages.emplace_back(message);
        }

        m_coder->encode(replay_messages, m_out_buffer);  // 编码回应数据
        // 启动写事件监听
        listenWrite();
    } else {
        vector<AbstractProtocol::s_ptr> result;
        m_coder->decode(result, m_in_buffer);  // 解码输入数据

        for (size_t i = 0; i < result.size(); ++i) {
            string req_id = result[i]->m_req_id;
            auto it = m_read_dones.find(req_id);
            if (it != m_read_dones.end()) {
                it->second(result[i]);  // 调用回调函数
            }
        }
    }
}

// 处理写入事件，将数据写入文件描述符
void TcpConnection::onWrite() {
    // 检查连接状态是否已关闭
    if (m_state != Connected) {
        ERRORLOG("onWrite error, client has already disconnected, addr[%s], clientfd[%d]", 
                 m_peer_addr->toString().c_str(), m_fd);
        return;
    }

    if (m_connection_type == TcpConnectionByClient) {
        vector<AbstractProtocol::s_ptr> messages;

        // 将待发送的消息放入列表
        for (size_t i = 0; i < m_write_dones.size(); ++i) {
            messages.push_back(m_write_dones[i].first);
        }

        m_coder->encode(messages, m_out_buffer);  // 编码待发送的数据
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
            // 写入数据时发生 EAGAIN 错误，说明写入缓冲区已满
            ERRORLOG("write data error, errno == EAGAIN and rt == -1");
            break;
        }
    }

    // 如果所有数据都已写入，取消写事件的监听
    if (is_write_all) {
        m_fd_event->cancel(FdEvent::OUT_EVENT);
        m_event_loop->addEpollEvent(m_fd_event);
    }

    // 如果是客户端连接，调用回调函数并清空写操作完成的列表
    if (m_connection_type == TcpConnectionByClient) {
        for (size_t i = 0; i < m_write_dones.size(); ++i) {
            m_write_dones[i].second(m_write_dones[i].first);
        }

        m_write_dones.clear();
    }
}

// 设置 TCP 连接的状态
void TcpConnection::setState(const TcpState state) {
    m_state = state;
}

// 获取 TCP 连接的状态
TcpState TcpConnection::getState() {
    return m_state;
}

// 清理 TCP 连接，关闭文件描述符并更新状态
void TcpConnection::clear() {
    if (m_state == Closed) {
        return;  // 如果已关闭，无需进一步处理
    }

    m_fd_event->cancel(FdEvent::IN_EVENT);
    m_fd_event->cancel(FdEvent::OUT_EVENT);

    // 从事件循环中删除 FdEvent 对象
    m_event_loop->deleteEpollEvent(m_fd_event);

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

// 设置 TCP 连接的类型
void TcpConnection::setConnectionType(TcpConnectionType type) {
    m_connection_type = type;
}

// 启动监听可写事件
void TcpConnection::listenWrite() {
    m_fd_event->listen(FdEvent::OUT_EVENT, bind(&TcpConnection::onWrite, this));
    m_event_loop->addEpollEvent(m_fd_event); 
}

// 启动监听可读事件
void TcpConnection::listenRead() {
    m_fd_event->listen(FdEvent::IN_EVENT, bind(&TcpConnection::onRead, this));
    m_event_loop->addEpollEvent(m_fd_event);
}

// 将要发送的消息和完成回调函数添加到发送队列
void TcpConnection::pushSendMessage(AbstractProtocol::s_ptr message, function<void(AbstractProtocol::s_ptr)> done) {
    m_write_dones.push_back(make_pair(message, done));
}

// 将要读取的消息的请求 ID 和完成回调函数添加到读取队列
void TcpConnection::pushReadMessage(const string& req_id, function<void(AbstractProtocol::s_ptr)> done) {
    m_read_dones.insert(make_pair(req_id, done));
}

}
