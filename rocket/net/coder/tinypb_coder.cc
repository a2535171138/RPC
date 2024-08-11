#include <string.h>
#include <arpa/inet.h> // 提供 htonl 和 ntohl 等网络字节序函数
#include "rocket/common/util.h"
#include "rocket/common/log.h"
#include "rocket/net/coder/tinypb_coder.h"
#include "rocket/net/coder/tinypb_protocol.h"

namespace rocket {

  // 编码函数，将一组消息编码到指定的缓冲区
  void TinyPBCoder::encode(vector<AbstractProtocol::s_ptr>& messages, TcpBuffer::s_ptr out_buffer) {
    for (auto &i : messages) {

      // 将 AbstractProtocol 类型的指针转换为 TinyPBProtocol 类型的指针
      shared_ptr<TinyPBProtocol> msg = dynamic_pointer_cast<TinyPBProtocol>(i);

      int len = 0;
      // 调用 encodeTinyPB 函数将消息编码为字节流，并获得字节流的长度
      const char* buf = encodeTinyPB(msg, len);
      
      // 如果编码成功，且字节流不为空，将其写入输出缓冲区
      if (buf != NULL && len != 0) {
        out_buffer->writeToBuffer(buf, len);
      }

      // 释放编码过程中分配的内存
      if (buf) {
        free((void*)buf);
        buf = NULL;
      }
    }
  }

  // 解码函数，从缓冲区中解码消息
  void TinyPBCoder::decode(vector<AbstractProtocol::s_ptr>& out_messages, TcpBuffer::s_ptr buffer) {
    while (1) {
      // 复制缓冲区中的数据到临时变量
      vector<char> tmp = buffer->m_buffer; 
      int start_index = buffer->readIndex(); // 获取当前读取的起始索引
      int end_index = -1; // 初始化结束索引为-1

      int pk_len = 0; // 协议长度
      bool parse_success = false; // 解析成功标志
      int i = 0;
      
      // 遍历缓冲区数据以查找协议的开始和结束标记
      for (i = start_index; i < buffer->writeIndex(); ++i) {
        // 找到协议的开始标记
        if (tmp[i] == TinyPBProtocol::PB_START) {
          // 从协议开始标记后获取协议长度
          pk_len = getInt32FromNetByte(&tmp[i + 1]);
          DEBUGLOG("get pk_len = %d", pk_len);

          int j = i + pk_len - 1; // 计算协议的结束索引
          // 如果结束索引超出了缓冲区的有效范围，继续寻找
          if (j >= buffer->writeIndex()) {
            continue;
          }

          // 如果找到协议的结束标记，标记为解析成功
          if (tmp[j] == TinyPBProtocol::PB_END) {
            start_index = i;
            end_index = j;
            parse_success = true;
            break;
          }
        }
      }

      // 如果遍历完缓冲区仍未找到完整协议，则退出解码
      if (i >= buffer->writeIndex()) {
        DEBUGLOG("decode end, read all buffer data");
        return;
      }

      // 如果解析成功，处理消息
      if (parse_success) {
        // 移动读取索引以跳过已经处理的协议数据
        buffer->moveReadIndex(end_index - start_index + 1);
        
        // 创建一个新的 TinyPBProtocol 对象
        shared_ptr<TinyPBProtocol> message = make_shared<TinyPBProtocol>();
        message->m_pk_len = pk_len; // 设置协议长度

        // 解析请求 ID 长度及其内容
        int req_id_len_index = start_index + sizeof(char) + sizeof(message->m_pk_len);
        if (req_id_len_index >= end_index) {
          message->parse_success = false;
          ERRORLOG("parse error, req_id_len_index[%d] >= end_index[%d]", req_id_len_index, end_index);
          continue;
        }
        message->m_req_id_len = getInt32FromNetByte(&tmp[req_id_len_index]);
        DEBUGLOG("parse req_id_len=%d", message->m_req_id_len);

        int req_id_index = req_id_len_index + sizeof(message->m_req_id_len);
        char req_id[100] = {0};
        memcpy(&req_id[0], &tmp[req_id_index], message->m_req_id_len);
        message->m_req_id = string(req_id);
        DEBUGLOG("parse req_id=%s", message->m_req_id.c_str());

        // 解析方法名称长度及其内容
        int method_name_len_index = req_id_index + message->m_req_id_len;
        if (method_name_len_index >= end_index) {
          message->parse_success = false;
          ERRORLOG("parse error, method_name_len_index[%d] >= end_index[%d]", method_name_len_index, end_index);
          continue;
        }
        message->m_method_name_len = getInt32FromNetByte(&tmp[method_name_len_index]);

        int method_name_index = method_name_len_index + sizeof(message->m_method_name_len);
        char method_name[512] = {0};
        memcpy(&method_name[0], &tmp[method_name_index], message->m_method_name_len);
        message->m_method_name = string(method_name);
        DEBUGLOG("parse method_name=%s", message->m_method_name.c_str());

        // 解析错误码
        int err_code_index = method_name_index + message->m_method_name_len;
        if (err_code_index >= end_index) {
          message->parse_success = false;
          ERRORLOG("parse error, err_code_index[%d] >= end_index[%d]", err_code_index, end_index);
          continue;
        }
        message->m_err_code = getInt32FromNetByte(&tmp[err_code_index]);

        // 解析错误信息长度及其内容
        int error_info_len_index = err_code_index + sizeof(message->m_err_code);
        if (error_info_len_index >= end_index) {
          message->parse_success = false;
          ERRORLOG("parse error, error_info_len_index[%d] >= end_index[%d]", error_info_len_index, end_index);
          continue;
        }
        message->m_err_info_len = getInt32FromNetByte(&tmp[error_info_len_index]);

        int err_info_index = error_info_len_index + sizeof(message->m_err_info_len);
        char error_info[512] = {0};
        memcpy(&error_info[0], &tmp[err_info_index], message->m_err_info_len);
        message->m_err_info = string(error_info);
        DEBUGLOG("parse error_info=%s", message->m_err_info.c_str());

        // 计算协议数据的长度并解析数据
        int pb_data_len = message->m_pk_len - message->m_method_name_len - message->m_req_id_len - message->m_err_info_len - 2 - 24;
        int pb_data_index = err_info_index + message->m_err_info_len;
        message->m_pb_data = string(&tmp[pb_data_index], pb_data_len);

        message->parse_success = true; // 设置解析成功标志
        out_messages.push_back(message); // 将解析后的消息添加到结果列表中
      }
    }
  }

  // 编码 TinyPBProtocol 对象为 TinyPB 协议格式的字节流
  const char* TinyPBCoder::encodeTinyPB(shared_ptr<TinyPBProtocol> message, int& len) {
    if (message->m_req_id.empty()) {
      message->m_req_id = "123456789"; // 如果请求 ID 为空，设置默认值
    }
    DEBUGLOG("req_id = %s", message->m_req_id.c_str());

    // 计算协议的总长度
    int pk_len = 2 + 24 + message->m_req_id.length() + message->m_method_name.length() + message->m_err_info.length() + message->m_pb_data.length();
    DEBUGLOG("pk_len = %d", pk_len);

    // 分配内存以存储编码后的数据
    char* buf = reinterpret_cast<char*>(malloc(pk_len));
    char* tmp = buf;

    // 写入协议开始标记
    *tmp = TinyPBProtocol::PB_START;
    tmp++;

    // 写入协议长度（网络字节序）
    int32_t pk_len_net = htonl(pk_len);
    memcpy(tmp, &pk_len_net, sizeof(pk_len_net));
    tmp += sizeof(pk_len_net);

    // 写入请求 ID 长度（网络字节序）
    int req_id_len = message->m_req_id.length();
    int32_t req_id_len_net = htonl(req_id_len);
    memcpy(tmp, &req_id_len_net, sizeof(req_id_len_net));
    tmp += sizeof(req_id_len_net);

    // 写入请求 ID
    if (!message->m_req_id.empty()) {
      memcpy(tmp, &(message->m_req_id[0]), req_id_len);
      tmp += req_id_len;
    }

    // 写入方法名称长度（网络字节序）
    int method_name_len = message->m_method_name.length();
    int32_t method_name_len_net = htonl(method_name_len);
    memcpy(tmp, &method_name_len_net, sizeof(method_name_len_net));
    tmp += sizeof(method_name_len_net);

    // 写入方法名称
    if (!message->m_method_name.empty()) {
      memcpy(tmp, &(message->m_method_name[0]), method_name_len);
      tmp += method_name_len;
    }

    // 写入错误码（网络字节序）
    int32_t err_code_net = htonl(message->m_err_code);
    memcpy(tmp, &err_code_net, sizeof(err_code_net));
    tmp += sizeof(err_code_net);

    // 写入错误信息长度（网络字节序）
    int err_info_len = message->m_err_info.length();
    int32_t err_info_len_net = htonl(err_info_len);
    memcpy(tmp, &err_info_len_net, sizeof(err_info_len_net));
    tmp += sizeof(err_info_len_net);

    // 写入错误信息
    if (!message->m_err_info.empty()) {
      memcpy(tmp, &(message->m_err_info[0]), err_info_len);
      tmp += err_info_len;
    }

    // 写入协议数据
    if (!message->m_pb_data.empty()) {
      memcpy(tmp, &(message->m_pb_data[0]), message->m_pb_data.length());
      tmp += message->m_pb_data.length();
    }

    // 写入校验和（设置为 1，网络字节序）
    int32_t check_sum_net = htonl(1);
    memcpy(tmp, &check_sum_net, sizeof(check_sum_net));
    tmp += sizeof(check_sum_net);

    // 写入协议结束标记
    *tmp = TinyPBProtocol::PB_END;

    // 更新消息的长度信息
    message->m_pk_len = pk_len;
    message->m_req_id_len = req_id_len;
    message->m_method_name_len = method_name_len;
    message->m_err_info_len = err_info_len;
    message->parse_success = true;
    len = pk_len;

    DEBUGLOG("encode message[%s] success", message->m_req_id.c_str());
    return buf;
  }
}
