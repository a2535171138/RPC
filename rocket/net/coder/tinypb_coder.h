#ifndef ROCKET_NET_CODER_TINYPB_CODER_H
#define ROCKET_NET_CODER_TINYPB_CODER_H

#include "rocket/net/coder/abstract_coder.h"
#include "rocket/net/coder/tinypb_protocol.h"

namespace rocket {

class TinyPBCoder : public AbstractCoder {
  public:
    TinyPBCoder() {}
    ~TinyPBCoder() {}

    // 编码方法，将消息编码为 TinyPB 协议并写入到输出缓冲区
    void encode(vector<AbstractProtocol::s_ptr>& messages, TcpBuffer::s_ptr out_buffer);

    // 解码方法，从输入缓冲区读取 TinyPB 协议数据并解析为消息
    void decode(vector<AbstractProtocol::s_ptr>& out_messages, TcpBuffer::s_ptr buffer);

  private:
    // 辅助函数，将 TinyPBProtocol 对象编码为二进制数据
    const char* encodeTinyPB(shared_ptr<TinyPBProtocol> message, int& len);
};

}

#endif
