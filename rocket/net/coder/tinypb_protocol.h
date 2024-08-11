#ifndef ROCKET_NET_CODER_TINYPB_PROTOCOL_H
#define ROCKET_NET_CODER_TINYPB_PROTOCOL_H

#include <string>                   // 提供 std::string 类型
#include "rocket/net/coder/abstract_protocol.h" // 提供 AbstractProtocol 类的基类

using namespace std;

namespace rocket {

// TinyPBProtocol 结构体继承自 AbstractProtocol
// 用于定义 TinyPB 编码协议的数据结构
struct TinyPBProtocol : public AbstractProtocol {
  public:
    TinyPBProtocol() {}            // 默认构造函数
    ~TinyPBProtocol() {}           // 默认析构函数

    static char PB_START;          // 协议开始标志
    static char PB_END;            // 协议结束标志

  public:
    int32_t m_pk_len {0};          // 包长度（包括协议头和数据）
    int32_t m_req_id_len {0};      // 请求 ID 的长度

    int32_t m_method_name_len {0}; // 方法名称的长度
    string m_method_name;          // 方法名称
    int32_t m_err_code {0};        // 错误码
    int32_t m_err_info_len {0};    // 错误信息的长度
    string m_err_info;             // 错误信息
    string m_pb_data;              // Protocol Buffers 数据
    int32_t m_check_sum {0};       // 校验和

    bool parse_success {false};    // 解析是否成功的标志
};

}

#endif
