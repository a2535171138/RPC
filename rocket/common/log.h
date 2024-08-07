// 日志系统
#ifndef ROCKET_COMMON_LOG_H
#define ROCKET_COMMON_LOG_H

#include <string>
#include <vector>
#include <queue>
#include <memory>
#include "rocket/common/config.h" // 配置管理
#include "rocket/common/mutex.h"  // 互斥锁

using namespace std;

namespace rocket {

// 日志的宏定义
#define DEBUGLOG(str, ...) \
  if (rocket::Logger::GetGlobalLogger()->getLogLevel() && rocket::Logger::GetGlobalLogger()->getLogLevel() <= rocket::Debug) \
  { \
    rocket::Logger::GetGlobalLogger()->pushLog(rocket::LogEvent(rocket::LogLevel::Debug).toString() \
      + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + rocket::formatString(str, ##__VA_ARGS__) + "\n");\
    rocket::Logger::GetGlobalLogger()->log(); \
  } \

#define INFOLOG(str, ...) \
  if (rocket::Logger::GetGlobalLogger()->getLogLevel() <= rocket::Info) \
  { \
    rocket::Logger::GetGlobalLogger()->pushLog(rocket::LogEvent(rocket::LogLevel::Info).toString() \
    + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + rocket::formatString(str, ##__VA_ARGS__) + "\n");\
    rocket::Logger::GetGlobalLogger()->log(); \
  } \

#define ERRORLOG(str, ...) \
  if (rocket::Logger::GetGlobalLogger()->getLogLevel() <= rocket::Error) \
  { \
    rocket::Logger::GetGlobalLogger()->pushLog(rocket::LogEvent(rocket::LogLevel::Error).toString() \
      + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + rocket::formatString(str, ##__VA_ARGS__) + "\n");\
  } \

// 字符串格式化函数模板
template<typename... Args>
std::string formatString(const char* str, Args&&... args) {
  int size = snprintf(nullptr, 0, str, args...);  // 获取格式化后的字符串长度

  std::string result;
  if (size > 0) {
    result.resize(size);  // 调整字符串大小
    snprintf(&result[0], size + 1, str, args...); // 格式化字符串
  }

  return result;
}

// 日志级别枚举
enum LogLevel {
  Unkown = 0,  // 未知级别
  Debug = 1,   // 调试级别
  Info = 2,    // 信息级别
  Error = 3    // 错误级别
};

// 日志记录器类
class Logger {
  public:
    // s_ptr为Logger类的智能指针类型
    typedef shared_ptr<Logger> s_ptr;

    // 构造函数，指定日志级别
    Logger(LogLevel level) : m_set_level(level){}

    // 推送日志消息到缓冲区
    void pushLog(const std::string& msg);

    // 获取全局唯一的日志记录器实例
    static Logger* GetGlobalLogger();
    // 初始化全局日志记录器
    static Logger* InitGlobalLogger();

    // 处理并输出日志消息
    void log();

    LogLevel getLogLevel() const{
      return m_set_level;
    }

  private:
    LogLevel m_set_level; // 当前设置的日志级别
    std::queue<string> m_buffer;  // 日志消息缓冲区
    Mutex m_mutex;

};

string LogLevelToString(LogLevel level);  // 将日志级别转换为字符串
LogLevel StringToLogLevel(const string& LogLevel);  // 将字符串转换为日志级别枚举

// 日志事件类
class LogEvent {
  public:
    // 构造函数，初始化日志级别
    LogEvent(LogLevel level):m_level(level){}

    // 获取文件名
    string getFileName() const {
      return m_file_name;
    }

    // 获取日志级别
    LogLevel getLogLevel() const {
      return m_level;
    }

    // 生成包含详细信息的日志消息字符串
    string toString();

  private:
    string m_file_name;   // 文件名
    int32_t m_file_line;  // 行号
    int32_t m_pid;        // 进程ID
    int32_t m_thread_id;  // 线程ID
    LogLevel m_level;     // 日志级别
};

}

#endif