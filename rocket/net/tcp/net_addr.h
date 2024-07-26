#ifndef ROCKET_NET_TCP_NET_ADDR_H
#define ROCKET_NET_TCP_NET_ADDR_H

#include <arpa/inet.h>  // 引入用于IPv4地址转换的头文件
#include <netinet/in.h>  // 引入用于定义网络地址结构的头文件
#include <string>  // 引入标准库中的string头文件
#include <memory>  // 引入标准库中的memory头文件

using namespace std;  // 使用标准命名空间

namespace rocket {  // 定义命名空间rocket

class NetAddr {  // 定义NetAddr基类
  public:
    typedef shared_ptr<NetAddr> s_ptr;  // 定义一个智能指针类型，指向NetAddr

    virtual sockaddr* getSockAddr() = 0;  // 纯虚函数，返回sockaddr指针

    virtual socklen_t getSockLen() = 0;  // 纯虚函数，返回sockaddr长度

    virtual int getFamily() = 0;  // 纯虚函数，返回地址族

    virtual string toString() = 0;  // 纯虚函数，返回字符串表示

    virtual bool checkValid() = 0;  // 纯虚函数，检查地址是否有效
};

class IPNetAddr : public NetAddr {  // 定义IPNetAddr类，继承自NetAddr
  public:
    IPNetAddr(const string &ip, uint16_t port);  // 构造函数，初始化IP和端口
    IPNetAddr(const string &addr);  // 构造函数，初始化地址
    IPNetAddr(sockaddr_in addr);  // 构造函数，使用sockaddr_in初始化

    sockaddr* getSockAddr();  // 重写getSockAddr函数

    socklen_t getSockLen();  // 重写getSockLen函数

    int getFamily();  // 重写getFamily函数

    string toString();  // 重写toString函数

    bool checkValid();  // 重写checkValid函数

  private:
    string m_ip;  // IP地址
    uint16_t m_port {0};  // 端口，初始值为0
    sockaddr_in m_addr;  // IPv4地址结构体
};

}

#endif  // 结束预处理指令，防止重复包含
