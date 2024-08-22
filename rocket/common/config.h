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
    string m_log_level; // 配置日志级别
    string m_log_file_name; // 日志文件名称
    string m_log_file_path; // 日志文件路径
    int m_log_max_file_size {0}; // 日志文件的最大大小，单位：字节
    int m_log_sync_interval {0}; // 日志同步间隔，单位：毫秒

  // private:
};

}

#endif
