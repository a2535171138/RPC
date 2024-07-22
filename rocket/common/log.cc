// 日志系统
#include <sys/time.h>
#include <sstream>
#include <stdio.h>
#include "rocket/common/log.h"
#include "rocket/common/util.h"

namespace rocket {

// 全局唯一的日志记录器实例
static Logger* g_logger = nullptr;

// 获取日志记录器
Logger* Logger::GetGlobalLogger(){
  return g_logger;
}

// 初始化全局日志记录器
Logger* Logger::InitGlobalLogger(){
  LogLevel global_log_level = StringToLogLevel(Config::GetGlobalConfig()->m_log_level); // 从全局配置获取日志级别
  printf("Init log level [%s]\n", LogLevelToString(global_log_level).c_str());  // 打印初始化日志级别信息
  // 创建新的实例返回
  g_logger = new Logger(global_log_level);
}


// 将日志级别枚举值转换为字符串
string LogLevelToString(LogLevel level){
  switch(level){
    case Debug:
      return "DEBUG";
    case Info:
      return "INFO";
    case Error:
      return "ERROR";
    default:
      return "UNKNOWN";
  }
}

// 将字符串转换为日志级别枚举
LogLevel StringToLogLevel(const string& LogLevel){
  if(LogLevel == "DEBUG") return Debug;
  else if(LogLevel == "INFO") return Info;
  else if(LogLevel == "ERROR") return Error;
  else return Debug;
}


// 生成包含详细信息的日志消息字符串
string LogEvent::toString(){
  // 获取当前时间
  struct timeval now_time;
  gettimeofday(&now_time, nullptr);
  // 转换为本地时间
  struct tm now_time_t;
  localtime_r(&(now_time.tv_sec), &now_time_t);
  // 格式化时间
  char buf[128];
  strftime(&buf[0], 128, "%y-%m-%d %H:%M:%S", &now_time_t);
  string time_str(buf);
  // 获取毫秒部分
  int ms = now_time.tv_usec / 1000;
  time_str = time_str + "." + to_string(ms);

  m_pid = getPid(); // 获取进程ID
  m_thread_id = getThreadId();  // 获取线程ID

  stringstream ss;  // 字符串流

  ss << "[" << LogLevelToString(m_level) << "]\t" // 日志级别
    << "[" << time_str << "]\t" // 时间
    << "[" << m_pid << ":" << m_thread_id << "]\t"; // 进程ID和线程ID

  return ss.str();  // 返回生成的日志消息字符串
}

// 将日志消息推送到缓冲区
void Logger::pushLog(const std::string& msg ){
  ScopeMutex<Mutex> lock(m_mutex);  // 加锁
  m_buffer.push(msg); // 将消息推入缓冲区
  lock.unlock();  // 解锁
}

// 处理并输出日志消息
void Logger::log(){
  ScopeMutex<Mutex> lock(m_mutex);  // 加锁
  queue<string> tmp; // 复制缓冲区消息
  m_buffer.swap(tmp); // 交换缓冲区
  lock.unlock();  // 解锁

  while(!tmp.empty()){
    string msg = tmp.front();  // 获取缓冲区中的第一条消息
    tmp.pop(); // 弹出已处理的消息
    printf(msg.c_str());  // 输出消息
  }
}

} 