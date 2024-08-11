#ifndef ROCKET_NET_STRING_CODER_H
#define ROCKET_NET_STRING_CODER_H

#include "rocket/net/coder/abstract_coder.h"
#include "rocket/net/coder/abstract_protocol.h"

namespace rocket {

// 表示字符串协议的类，继承自 AbstractProtocol
// 该协议包含一个字符串信息字段
class StringProtocol : public AbstractProtocol {
  public:
    string info; // 存储字符串信息
};

// 表示字符串编码器的类，继承自 AbstractCoder
// 该编码器用于将 StringProtocol 消息编码到缓冲区中，以及从缓冲区中解码
class StringCoder : public AbstractCoder {
  public:
    // 实现 AbstractCoder 的 encode 方法
    // 将一组 AbstractProtocol 消息编码到 TcpBuffer 中
    void encode(vector<AbstractProtocol::s_ptr>& messages, TcpBuffer::s_ptr out_buffer) override {
      // 遍历每个消息，进行编码
      for(size_t i = 0; i < messages.size(); ++i) {
        // 将消息转换为 StringProtocol 类型
        shared_ptr<StringProtocol> msg = dynamic_pointer_cast<StringProtocol>(messages[i]);
        // 将字符串信息写入到缓冲区
        out_buffer->writeToBuffer(msg->info.c_str(), msg->info.length());
      }
    }

    // 实现 AbstractCoder 的 decode 方法
    // 从 TcpBuffer 中解码出一组 AbstractProtocol 消息
    void decode(vector<AbstractProtocol::s_ptr>& out_messages, TcpBuffer::s_ptr buffer) override {
      // 从缓冲区读取数据到临时字符向量中
      vector<char> re;
      buffer->readFromBuffer(re, buffer->readAble());
      // 将字符向量拼接成字符串
      string info;
      for(size_t i = 0; i < re.size(); ++i) {
        info += re[i];
      }

      // 创建一个新的 StringProtocol 消息对象
      shared_ptr<StringProtocol> msg = make_shared<StringProtocol>();
      msg->info = info; // 设置消息中的字符串信息
      msg->m_req_id = "123456"; // 设置请求ID
      out_messages.push_back(msg); // 将消息对象添加到输出消息集合中
    }
};

}

#endif
