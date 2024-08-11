#include "rocket/net/coder/tinypb_protocol.h"

namespace rocket {
  // 定义 TinyPBProtocol 的静态成员 PB_START 和 PB_END
  char TinyPBProtocol::PB_START = 0x02; // 协议开始标志，设置为 0x02
  char TinyPBProtocol::PB_END = 0x03;   // 协议结束标志，设置为 0x03
}
