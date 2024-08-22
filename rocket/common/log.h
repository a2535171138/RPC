// 日志系统头文件
#ifndef ROCKET_COMMON_LOG_H
#define ROCKET_COMMON_LOG_H

#include <string>
#include <vector>
#include <queue>
#include <memory>
#include <semaphore.h>
#include "rocket/common/config.h" // 配置管理
#include "rocket/common/mutex.h"  // 互斥锁
#include "rocket/net/timer_event.h" 

using namespace std;

namespace rocket {

// 日志的宏定义，用于不同级别的日志记录
#define DEBUGLOG(str, ...) \
  if (rocket::Logger::GetGlobalLogger()->getLogLevel() && rocket::Logger::GetGlobalLogger()->getLogLevel() <= rocket::Debug) \
  { \
    rocket::Logger::GetGlobalLogger()->pushLog(rocket::LogEvent(rocket::LogLevel::Debug).toString() \
      + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + rocket::formatString(str, ##__VA_ARGS__) + "\n");\
  } \

#define INFOLOG(str, ...) \
  if (rocket::Logger::GetGlobalLogger()->getLogLevel() <= rocket::Info) \
  { \
    rocket::Logger::GetGlobalLogger()->pushLog(rocket::LogEvent(rocket::LogLevel::Info).toString() \
    + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + rocket::formatString(str, ##__VA_ARGS__) + "\n");\
  } \

#define ERRORLOG(str, ...) \
  if (rocket::Logger::GetGlobalLogger()->getLogLevel() <= rocket::Error) \
  { \
    rocket::Logger::GetGlobalLogger()->pushLog(rocket::LogEvent(rocket::LogLevel::Error).toString() \
      + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + rocket::formatString(str, ##__VA_ARGS__) + "\n");\
  } \

#define APPDEBUGLOG(str, ...) \
  if (rocket::Logger::GetGlobalLogger()->getLogLevel() && rocket::Logger::GetGlobalLogger()->getLogLevel() <= rocket::Debug) \
  { \
    rocket::Logger::GetGlobalLogger()->pushAppLog(rocket::LogEvent(rocket::LogLevel::Debug).toString() \
      + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + rocket::formatString(str, ##__VA_ARGS__) + "\n");\
  } \

#define APPINFOLOG(str, ...) \
  if (rocket::Logger::GetGlobalLogger()->getLogLevel() <= rocket::Info) \
  { \
    rocket::Logger::GetGlobalLogger()->pushAppLog(rocket::LogEvent(rocket::LogLevel::Info).toString() \
    + "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" + rocket::formatString(str, ##__VA_ARGS__) + "\n");\
  } \

#define APPERRORLOG(str, ...) \
  if (rocket::Logger::GetGlobalLogger()->getLogLevel() <= rocket::Error) \
  { \
    rocket::Logger::GetGlobalLogger()->pushAppLog(rocket::LogEvent(rocket::LogLevel::Error).toString() \
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

// 日志级别枚举定义
enum LogLevel {
  Unkown = 0,  // 未知级别
  Debug = 1,   // 调试级别
  Info = 2,    // 信息级别
  Error = 3    // 错误级别
};

// 异步日志类
class AsyncLogger {
  public:
    typedef shared_ptr<AsyncLogger> s_ptr;

    // 构造函数，初始化日志文件名、路径和最大大小
    AsyncLogger(const string& file_name, const string& file_path, int max_size);

    // 停止日志记录
    void stop();

    // 刷新日志缓冲区到磁盘
    void flush();

    // 推送日志缓冲区
    void pushLogBuffer(vector<string>& vec);

    // 日志循环，负责不断将日志写入磁盘
    static void* Loop(void*);

  private:
    queue<vector<string>> m_buffer;  // 日志缓冲区队列

    string m_file_name;  // 日志文件名
    string m_file_path;  // 日志文件路径
    int m_max_file_size {0};  // 日志单个文件的最大大小，单位为字节

    sem_t m_semaphore;  // 信号量，用于同步
    pthread_t m_thread;  // 线程对象

    pthread_cond_t m_condition;  // 条件变量，用于线程同步
    Mutex m_mutex;  // 互斥锁，用于保护共享资源

    string m_date;  // 当前日志文件的日期
    FILE* m_file_handler {NULL};  // 当前打开的日志文件句柄

    bool m_reopen_flag {false};  // 文件重新打开标志

    int m_no {0};  // 日志文件序号

    bool m_stop_flag {false};  // 停止标志
};

// 日志记录器类
class Logger {
  public:
    typedef shared_ptr<Logger> s_ptr;

    // 构造函数，初始化日志级别
    Logger(LogLevel level);

    // 推送日志消息到缓冲区
    void pushLog(const std::string& msg);

    void pushAppLog(const std::string& msg);

    // 初始化日志记录器
    void init();

    // 获取全局唯一的日志记录器实例
    static Logger* GetGlobalLogger();
    // 初始化全局日志记录器实例
    static Logger* InitGlobalLogger();

    // 处理并输出日志消息
    void log();

    // 获取当前的日志级别
    LogLevel getLogLevel() const{
      return m_set_level;
    }

    // 日志同步循环，定时将缓冲区日志写入磁盘
    void syncLoop();

  private:
    LogLevel m_set_level;  // 当前设置的日志级别
    std::vector<string> m_buffer;  // 日志消息缓冲区
    std::vector<string> m_app_buffer;  // 应用日志消息缓冲区
    Mutex m_mutex;  // 互斥锁，用于保护日志缓冲区
    Mutex m_app_mutex;  // 互斥锁，用于保护应用日志缓冲区

    string m_file_name;  // 日志输出文件名
    string m_file_path;  // 日志输出路径
    int m_max_file_size {0};  // 日志单个文件的最大大小，单位为字节

    AsyncLogger::s_ptr m_asnyc_logger;  // 异步日志记录器指针
    AsyncLogger::s_ptr m_asnyc_app_logger;  // 应用异步日志记录器指针
    TimerEvent::s_ptr m_timer_event;  // 定时事件指针
};

// 辅助函数：将日志级别转换为字符串
string LogLevelToString(LogLevel level);
// 辅助函数：将字符串转换为日志级别枚举
LogLevel StringToLogLevel(const string& LogLevel);

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
