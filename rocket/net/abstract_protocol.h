#ifndef ROCKET_NET_ABSTRACT_PROTOCOL_H
#define ROCKET_NET_ABSTRACT_PROTOCOL_H

#include <memory>

namespace rocket{

// 抽象协议类，所有的协议类都将继承自这个类
class AbstractProtocol : public enable_shared_from_this<AbstractProtocol>{
  public:
    // 定义一个智能指针类型，便于后续管理对象的生命周期
    typedef std::shared_ptr<AbstractProtocol> s_ptr;
    
    // 获取请求ID的方法
    string getReqId(){
      return m_req_id;
    }

    // 设置请求ID的方法
    void setReqId(const string& req_id){
      m_req_id = req_id;
    }

    // 虚析构函数，确保派生类的析构函数能被正确调用
    virtual ~AbstractProtocol(){}

  protected:
    // 请求ID，唯一标识一个请求或响应
    string m_req_id;  

};

}

#endif
