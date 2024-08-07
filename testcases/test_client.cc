#include <assert.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <memory>
#include <unistd.h>
#include "rocket/common/log.h"
#include "rocket/common/config.h"

// 测试连接函数
void test_connect() {
  // 创建一个 TCP 套接字
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  
  // 检查套接字是否创建成功
  if (fd < 0) {
    ERRORLOG("invalid fd %d", fd);  // 记录错误日志
    exit(0);  // 退出程序
  }

  // 设置服务器地址
  sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));  // 清零结构体
  server_addr.sin_family = AF_INET;  // 地址族
  server_addr.sin_port = htons(12350);  // 端口号，使用网络字节顺序
  inet_aton("127.0.0.1", &server_addr.sin_addr);  // 设置 IP 地址为本地地址

  // 连接到服务器
  int rt = connect(fd, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));

  // 检查连接是否成功
  if (rt < 0) {
    ERRORLOG("connect failed, fd[%d]", fd);  // 记录错误日志
    close(fd);  // 关闭套接字
    return;
  }

  // 发送数据到服务器
  string msg = "hello rocket!";
  rt = write(fd, msg.c_str(), msg.length());  // 写入数据
  DEBUGLOG("success write %d bytes, [%s]", rt, msg.c_str());  // 记录调试日志

  // 从服务器读取数据
  char buf[100];  // 数据缓冲区
  rt = read(fd, buf, 100);  // 读取数据
  if (rt < 0) {
    ERRORLOG("read failed, fd[%d]", fd);  // 记录错误日志
  } else {
    DEBUGLOG("success read %d bytes, [%s]", rt, string(buf).c_str());  // 记录调试日志
  }

  // 关闭套接字
  close(fd);
}

int main() {
  // 禁用 stdout 的缓冲区，这样 printf 直接输出到终端
  setvbuf(stdout, NULL, _IONBF, 0);

  // 设置全局配置文件路径
  rocket::Config::SetGlobalConfig("../conf/rocket.xml");

  // 初始化全局日志记录器
  rocket::Logger::InitGlobalLogger();

  // 调用测试连接函数
  test_connect();

  return 0;
}
