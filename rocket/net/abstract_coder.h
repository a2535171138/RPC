#ifndef ROCKET_NET_ABSTRACT_CODER_H
#define ROCKET_NET_ABSTRACT_CODER_H

#include <vector>
#include "rocket/net/tcp/tcp_buffer.h"
#include "rocket/net/abstract_protocol.h"

namespace rocket{

// 抽象编码器类，定义了编码和解码操作的接口
class AbstractCoder{
  public:
    // 纯虚函数，负责将协议消息编码到TCP缓冲区中
    // 参数 messages 是要编码的协议消息集合，out_buffer 是输出的TCP缓冲区
    virtual void encode(vector<AbstractProtocol::s_ptr>& messages, TcpBuffer::s_ptr out_buffer) = 0;

    // 纯虚函数，负责从TCP缓冲区解码出协议消息
    // 参数 buffer 是输入的TCP缓冲区，out_messages 是解码后的协议消息集合
    virtual void decode(vector<AbstractProtocol::s_ptr>& out_messages, TcpBuffer::s_ptr buffer) = 0;

    // 虚析构函数，确保派生类的析构函数能被正确调用
    virtual ~AbstractCoder(){}
};

}
#endif
