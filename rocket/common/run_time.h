#ifndef ROCKET_COMMON_RUN_TIME_H
#define ROCKET_COMMON_RUN_TIME_H

#include <string>

using namespace std;

namespace rocket{

// 运行时类，用于在程序运行过程中存储和访问一些上下文信息
class RunTime{
  public:
    // 获取全局唯一的运行时实例
    static RunTime* GetRunTime();

    string m_msgid;         // 当前处理的消息ID
    string m_method_name;   // 当前执行的方法名称

};

}

#endif
