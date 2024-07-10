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
Logger* Logger::GetGlobolLogger(){
  // 返回已存在的实例
  if(g_logger){
    return g_logger;  
  }

  // 创建新的实例返回
  g_logger = new Logger();
  return g_logger;
}

// 将日志级别枚举值转换为字符串
string LogLevelToString(LogLevel level){
  switch(level){
    case Debug:
      return "Debug";
    case Info:
      return "Info";
    case Error:
      return "Error";
    default:
      return "Unknown";
  }
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

  stringstream ss;

  ss << "[" << LogLevelToString(m_level) << "]\t"
    << "[" << time_str << "]\t"
    << "[" << string(__FILE__) << ":" << __LINE__ << "]\t";


  return ss.str();

}

// 将日志消息推送到缓冲区
void Logger::pushLog(const std::string& msg ){
  m_buffer.push(msg);
}

// 处理并输出日志消息
void Logger::log(){
  while(!m_buffer.empty()){
    string msg = m_buffer.front();  // 获取缓冲区中的第一条消息
    m_buffer.pop(); // 弹出已处理的消息
    printf(msg.c_str());  // 输出消息
  }
}

} 