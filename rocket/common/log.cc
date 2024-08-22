// 日志系统
#include <sys/time.h>
#include <sstream>
#include <stdio.h>
#include <assert.h>
#include "rocket/common/log.h"
#include "rocket/common/util.h"
#include "rocket/common/config.h"
#include "rocket/net/eventloop.h"
#include "rocket/common/run_time.h"

namespace rocket {

// 全局唯一的日志记录器实例
static Logger* g_logger = nullptr;

// 获取全局日志记录器实例
Logger* Logger::GetGlobalLogger() {
  return g_logger;
}

// 构造函数，初始化日志记录器
Logger::Logger(LogLevel level) : m_set_level(level) {
  // 创建异步日志记录器实例，用于RPC日志
  m_asnyc_logger = make_shared<AsyncLogger>(
    Config::GetGlobalConfig()->m_log_file_name + "_rpc", // 日志文件名
    Config::GetGlobalConfig()->m_log_file_path,          // 日志文件路径
    Config::GetGlobalConfig()->m_log_max_file_size       // 日志文件最大大小
  );
  
  // 创建异步日志记录器实例，用于应用日志
  m_asnyc_app_logger = make_shared<AsyncLogger>(
    Config::GetGlobalConfig()->m_log_file_name + "_app", // 日志文件名
    Config::GetGlobalConfig()->m_log_file_path,          // 日志文件路径
    Config::GetGlobalConfig()->m_log_max_file_size       // 日志文件最大大小
  );
}

// 初始化日志系统
void Logger::init() {
  // 创建定时器事件，用于定期同步日志
  m_timer_event = std::make_shared<TimerEvent>(
    Config::GetGlobalConfig()->m_log_sync_interval,   // 同步间隔
    true,                                             // 是否循环触发
    std::bind(&Logger::syncLoop, this)                // 同步操作
  );
  EventLoop::GetCurrentEventLoop()->addTimerEvent(m_timer_event);
}

// 定期同步日志缓冲区到异步日志记录器
void Logger::syncLoop() {
  // 同步 m_buffer 到异步日志记录器的缓冲区
  printf("sync to async logger\n");
  vector<string> tmp_vec;
  ScopeMutex<Mutex> lock(m_mutex); // 互斥锁保护缓冲区
  tmp_vec.swap(m_buffer);          // 交换缓冲区内容
  lock.unlock();                   // 解锁

  if (!tmp_vec.empty()) {
    m_asnyc_logger->pushLogBuffer(tmp_vec); // 将日志推送到异步日志记录器
  }
  tmp_vec.clear();

  // 同步 m_app_buffer 到应用异步日志记录器的缓冲区
  vector<string> tmp_vec2;
  ScopeMutex<Mutex> lock2(m_app_mutex); // 互斥锁保护应用缓冲区
  tmp_vec2.swap(m_app_buffer);          // 交换应用缓冲区内容
  lock.unlock();                        // 解锁

  if (!tmp_vec2.empty()) {
    m_asnyc_app_logger->pushLogBuffer(tmp_vec2); // 将应用日志推送到异步日志记录器
  }
}

// 初始化全局日志记录器
Logger* Logger::InitGlobalLogger() {
  LogLevel global_log_level = StringToLogLevel(Config::GetGlobalConfig()->m_log_level); // 从全局配置获取日志级别
  printf("Init log level [%s]\n", LogLevelToString(global_log_level).c_str()); // 打印初始化日志级别信息
  // 创建新的实例返回
  g_logger = new Logger(global_log_level);
  g_logger->init(); // 初始化日志系统
}

// 将日志级别枚举值转换为字符串
string LogLevelToString(LogLevel level) {
  switch(level) {
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
LogLevel StringToLogLevel(const string& LogLevel) {
  if(LogLevel == "DEBUG") return Debug;
  else if(LogLevel == "INFO") return Info;
  else if(LogLevel == "ERROR") return Error;
  else return Debug;
}

// 生成包含详细信息的日志消息字符串
string LogEvent::toString() {
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
  m_thread_id = getThreadId(); // 获取线程ID

  stringstream ss; // 字符串流

  ss << "[" << LogLevelToString(m_level) << "]\t" // 日志级别
     << "[" << time_str << "]\t" // 时间
     << "[" << m_pid << ":" << m_thread_id << "]\t"; // 进程ID和线程ID

  // 获取当前线程处理的请求的 msgid
  string msgid = RunTime::GetRunTime()->m_msgid;
  string method_name = RunTime::GetRunTime()->m_method_name;
  if(!msgid.empty()) {
    ss << "[" << msgid << "]\t";
  }

  if(!method_name.empty()) {
    ss << "[" << method_name << "]\t";
  }

  return ss.str(); // 返回生成的日志消息字符串
}

// 将日志消息推送到缓冲区
void Logger::pushLog(const std::string& msg) {
  ScopeMutex<Mutex> lock(m_mutex); // 加锁
  m_buffer.push_back(msg); // 将消息推入缓冲区
  lock.unlock(); // 解锁
}

// 将应用日志消息推送到缓冲区
void Logger::pushAppLog(const std::string& msg) {
  ScopeMutex<Mutex> lock(m_app_mutex); // 加锁
  m_app_buffer.push_back(msg); // 将消息推入缓冲区
  lock.unlock(); // 解锁
}

// 空的log函数，用于拓展
void Logger::log() {

}

// 异步日志记录器的构造函数，初始化相关参数
AsyncLogger::AsyncLogger(const string& file_name, const string& file_path, int max_size)
 : m_file_name(file_name), m_file_path(file_path), m_max_file_size(max_size) {
  sem_init(&m_semaphore, 0, 0); // 初始化信号量

  assert(pthread_create(&m_thread, NULL, &AsyncLogger::Loop, this) == 0); // 创建异步日志线程

  sem_wait(&m_semaphore); // 等待线程初始化完成
}

// 异步日志线程的主循环
void* AsyncLogger::Loop(void* arg) {
  // 将缓冲区中的数据写入文件，然后线程睡眠，直到有新的数据再重复这个过程

  AsyncLogger* logger = reinterpret_cast<AsyncLogger*>(arg);

  assert(pthread_cond_init(&logger->m_condition, NULL) == 0); // 初始化条件变量

  sem_post(&logger->m_semaphore); // 通知主线程，子线程已初始化完成

  while(1) {
    ScopeMutex<Mutex> lock(logger->m_mutex); // 加锁
    while(logger->m_buffer.empty()) { // 如果缓冲区为空，则等待
      printf("begin pthread_cond_wait back \n");
      pthread_cond_wait(&(logger->m_condition), logger->m_mutex.getMutex());
    }
    printf("pthread_cond_wait back \n");

    vector<string> tmp;
    tmp.swap(logger->m_buffer.front()); // 交换缓冲区内容
    logger->m_buffer.pop(); // 弹出缓冲区内容

    lock.unlock(); // 解锁

    timeval now;
    gettimeofday(&now, NULL);

    struct tm now_time;
    localtime_r(&(now.tv_sec), &now_time);

    const char* format = "%Y%m%d";
    char date[32];
    strftime(date, sizeof(date), format, &now_time);

    // 如果日期发生变化，则重新打开新的日志文件
    if(string(date) != logger->m_date) {
      logger->m_no = 0;
      logger->m_reopen_flag = true;
      logger->m_date = string(date);
    }

    if(logger->m_file_handler == NULL) {
      logger->m_reopen_flag = true;
    }

    stringstream ss;
    ss << logger->m_file_path << logger->m_file_name << "_"
       << string(date) << "_log.";
    string log_file_name = ss.str() + to_string(logger->m_no);

    if(logger->m_reopen_flag) {
      if(logger->m_file_handler) {
        fclose(logger->m_file_handler); // 关闭旧的日志文件
      }
      logger->m_file_handler = fopen(log_file_name.c_str(), "a"); // 打开新的日志文件
      logger->m_reopen_flag = false;
    }

    if(logger->m_file_handler == NULL) {
      printf("Error: Failed to open log file: %s\n", log_file_name.c_str());
      continue; // 处理打开文件失败的情况
    }

    // 如果文件大小超过最大限制，重新创建新的日志文件
    if(ftell(logger->m_file_handler) > logger->m_max_file_size) {
      fclose(logger->m_file_handler); // 关闭当前文件

      log_file_name = ss.str() + to_string(logger->m_no++);
      logger->m_file_handler = fopen(log_file_name.c_str(), "a");
      logger->m_reopen_flag = false;
    }

    // 将缓冲区中的日志写入文件
    for(auto &i : tmp) {
      if(!i.empty()) {
        fwrite(i.c_str(), 1, i.length(), logger->m_file_handler);
      }
    }

    fflush(logger->m_file_handler); // 刷新文件缓冲区

    if(logger->m_stop_flag) {
      return NULL; // 退出线程
    }
  }

  return NULL;
}

// 停止异步日志记录器
void AsyncLogger::stop() {
  m_stop_flag = true;
}

// 刷新日志文件缓冲区
void AsyncLogger::flush() {
  if(m_file_handler) {
    fflush(m_file_handler);
  }
}

// 推送日志缓冲区到异步日志记录器
void AsyncLogger::pushLogBuffer(vector<string>& vec) {
  ScopeMutex<Mutex> lock(m_mutex); // 加锁
  m_buffer.push(vec); // 将日志缓冲区推送到队列
  pthread_cond_signal(&m_condition); // 唤醒异步日志线程

  lock.unlock(); // 解锁

  printf("pthread_cond_signal\n");
}

} // namespace rocket
