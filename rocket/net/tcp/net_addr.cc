#include <string.h>  // 引入标准库中的string.h头文件
#include "rocket/net/tcp/net_addr.h"  // 引入NetAddr头文件
#include "rocket/common/log.h"  // 引入日志头文件

namespace rocket {  // 定义命名空间rocket

IPNetAddr::IPNetAddr(const string &ip, uint16_t port) : m_ip(ip), m_port(port) {  // 构造函数，初始化IP和端口
  memset(&m_addr, 0, sizeof(m_addr));  // 将m_addr结构体清零

  m_addr.sin_family = AF_INET;  // 设置地址族为IPv4
  m_addr.sin_addr.s_addr = inet_addr(m_ip.c_str());  // 将IP地址转换为网络字节序并存储
  m_addr.sin_port = htons(m_port);  // 将端口转换为网络字节序并存储
}

IPNetAddr::IPNetAddr(const string &addr) {  // 构造函数，初始化地址
  size_t i = addr.find_first_of(":");  // 查找地址中的冒号位置
  if (i == addr.npos) {  // 如果未找到冒号
    ERRORLOG("invalid ipv4 addr %s", addr.c_str());  // 记录错误日志
    return;
  }

  m_ip = addr.substr(0, i);  // 截取冒号前的部分作为IP地址
  m_port = atoi(addr.substr(i + 1, addr.size() - i - 1).c_str());  // 截取冒号后的部分并转换为端口号

  memset(&m_addr, 0, sizeof(m_addr));  // 将m_addr结构体清零
  m_addr.sin_family = AF_INET;  // 设置地址族为IPv4
  m_addr.sin_addr.s_addr = inet_addr(m_ip.c_str());  // 将IP地址转换为网络字节序并存储
  m_addr.sin_port = htons(m_port);  // 将端口转换为网络字节序并存储
}

IPNetAddr::IPNetAddr(sockaddr_in addr) : m_addr(addr) {  // 构造函数，使用sockaddr_in结构体初始化
  m_ip = string(inet_ntoa(m_addr.sin_addr));  // 将网络字节序的IP地址转换为字符串并存储
  m_port = ntohs(m_addr.sin_port);  // 将网络字节序的端口转换为主机字节序并存储
}

sockaddr* IPNetAddr::getSockAddr() {  // 返回sockaddr指针
  return reinterpret_cast<sockaddr*>(&m_addr);
}

socklen_t IPNetAddr::getSockLen() {  // 返回sockaddr长度
  return sizeof(m_addr);
}

int IPNetAddr::getFamily() {  // 返回地址族
  return AF_INET;
}

string IPNetAddr::toString() {  // 返回字符串表示
  string re;
  re = m_ip + ":" + to_string(m_port);  // 拼接IP地址和端口号
  return re;
}

bool IPNetAddr::checkValid() {  // 检查地址是否有效
  if (m_ip.empty()) {  // 如果IP地址为空
    return false;
  }
  if (m_port < 0 || m_port > 65536) {  // 如果端口号不在有效范围内
    return false;
  }

  if (inet_addr(m_ip.c_str()) == INADDR_NONE) {  // 如果IP地址无效
    return false;
  }

  return true;  // 地址有效
}

}  // 命名空间结束
