#ifndef ROCKET_NET_ABSTRACT_PROTOCOL_H
#define ROCKET_NET_ABSTRACT_PROTOCOL_H

#include <memory>

using namespace std;

namespace rocket{

// 抽象协议类，所有的协议类都将继承自这个类
struct AbstractProtocol : public enable_shared_from_this<AbstractProtocol>{
  public:
    // 定义一个智能指针类型，便于后续管理对象的生命周期
    typedef std::shared_ptr<AbstractProtocol> s_ptr;

    // 虚析构函数，确保派生类的析构函数能被正确调用
    virtual ~AbstractProtocol(){}

  public:
    // 请求ID，唯一标识一个请求或响应
    string m_msg_id;  

};

}

#endif
