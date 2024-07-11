#ifndef ROCKET_COMMON_CONFIG_H
#define ROCKET_COMMON_CONFIG_H

#include <map>

using namespace std;

namespace rocket{

// 配置类，用于加载和管理配置信息
class Config {
  public:
    // 构造函数，从指定的 XML 文件加载配置
    Config(const char* xmlfile);
    // 获取全局配置实例
    static Config* GetGlobalConfig();
    // 设置全局配置实例
    static void SetGlobalConfig(const char* xmlfile);
    // 日志级别
    string m_log_level;

  // private:
};

}

#endif