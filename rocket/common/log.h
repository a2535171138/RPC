// 日志系统
#ifndef ROCKET_COMMON_LOG_H
#define ROCKET_COMMON_LOG_H

#include <string>
#include <vector>
#include <queue>
#include <memory>

using namespace std;

namespace rocket {

// DEBUG日志的宏定义
#define DEBUGLOG(str, ...)                                                          \
  rocket::LogEvent logEvent(rocket::LogLevel::Debug);                               \
  std::string msg = logEvent.toString() + rocket::formatString(str, ##__VA_ARGS__); \
  msg += "\n";                                                                      \
  rocket::Logger::GetGlobalLogger()->pushLog(msg);                                  \
  rocket::Logger::GetGlobalLogger()->log();                                         \

// 字符串格式化
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
  Debug = 1,
  Info = 2,
  Error = 3
};

class Logger {
  public:
    // s_ptr为Logger类的智能指针类型
    typedef shared_ptr<Logger> s_ptr;

    // 推送日志消息到缓冲区
    void pushLog(const std::string& msg);

    // 获取全局唯一的日志记录器实例
    static Logger* GetGlobalLogger();

    // 处理并输出日志消息
    void log();

  private:
    LogLevel m_set_level; // 当前设置的日志级别
    std::queue<string> m_buffer;  // 日志消息缓冲区

};


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