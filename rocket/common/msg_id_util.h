#ifndef ROCKET_COMMON_MSGID_UTIL_H
#define ROCKET_COMMON_MSGID_UTIL_H

#include <string>  // 引入标准库的字符串类

using namespace std;  // 使用标准命名空间中的所有成员

namespace rocket {

// 定义一个用于生成消息ID的工具类
class MsgIDUtil {
  public:
    // 静态成员函数，用于生成唯一的消息ID
    static string GenMsgID();
};

}  // namespace rocket

#endif  // ROCKET_COMMON_MSGID_UTIL_H
