#include <memory>  // 引入标准库中的memory头文件
#include <string.h>  // 引入标准库中的string.h头文件
#include "rocket/net/tcp/tcp_buffer.h"  // 引入TcpBuffer头文件
#include "rocket/common/log.h"  // 引入日志头文件

namespace rocket {  // 定义命名空间rocket

TcpBuffer::TcpBuffer(int size) : m_size(size) {  // 构造函数，初始化缓冲区大小并设置缓冲区大小
  m_buffer.resize(size);  // 调整缓冲区大小
}

TcpBuffer::~TcpBuffer() {  // 析构函数，释放资源

}

int TcpBuffer::readAble() {  // 返回当前可读数据的大小
  return m_write_index - m_read_index;  // 可读数据大小为写索引减去读索引
}

int TcpBuffer::writeAble() {  // 返回当前可写空间的大小
  return m_buffer.size() - m_write_index;  // 可写空间大小为缓冲区大小减去写索引
}

int TcpBuffer::readIndex() {  // 返回当前读索引
  return m_read_index;
}

int TcpBuffer::writeIndex() {  // 返回当前写索引
  return m_write_index;
}

void TcpBuffer::writeToBuffer(const char* buf, int size) {  // 将数据写入缓冲区
  if (size > writeAble()) {  // 如果要写入的数据大小大于可写空间大小
    int new_size = int(1.5 * (m_write_index + size));  // 计算新的缓冲区大小
    resizeBuffer(new_size);  // 调整缓冲区大小
  }

  memcpy(&m_buffer[m_write_index], buf, size);  // 将数据复制到缓冲区中
  m_write_index += size;
}

void TcpBuffer::readFromBuffer(vector<char>& re, int size) {  // 从缓冲区读取数据
  if (readAble() == 0) {  // 如果没有可读数据
    return;
  }

  int read_size = readAble() > size ? size : readAble();  // 计算实际读取的数据大小

  vector<char> tmp(read_size);  // 创建一个临时缓冲区
  memcpy(&tmp[0], &m_buffer[m_read_index], read_size);  // 将数据复制到临时缓冲区中

  re.swap(tmp);  // 交换缓冲区
  m_read_index += read_size;  // 移动读索引

  adjustBuffer();  // 调整缓冲区
}

void TcpBuffer::resizeBuffer(int new_size) {  // 调整缓冲区大小
  vector<char> tmp(new_size);  // 创建一个新的缓冲区
  int count = min(new_size, readAble());  // 计算要复制的数据大小

  memcpy(&tmp[0], &m_buffer[m_read_index], count);  // 将数据复制到新的缓冲区中
  m_buffer.swap(tmp);  // 交换缓冲区

  m_read_index = 0;  // 重置读索引
  m_write_index = m_read_index + count;  // 重置写索引
}

void TcpBuffer::adjustBuffer() {  // 调整缓冲区，使未读数据移到缓冲区起始位置
  if (m_read_index < int(m_buffer.size() / 3)) {  // 如果读索引小于缓冲区大小的三分之一
    return;
  }

  vector<char> buffer(m_buffer.size());  // 创建一个新的缓冲区
  int count = readAble();  // 计算未读数据大小

  memcpy(&buffer[0], &m_buffer[m_read_index], count);  // 将未读数据复制到新的缓冲区中
  m_buffer.swap(buffer);  // 交换缓冲区
  m_read_index = 0;  // 重置读索引
  m_write_index = m_read_index + count;  // 重置写索引

  buffer.clear();  // 清空临时缓冲区
}

void TcpBuffer::moveReadIndex(int size) {  // 移动读索引
  size_t j = m_read_index + size;  // 计算新的读索引
  if (j >= m_buffer.size()) {  // 如果新的读索引超过缓冲区大小
    ERRORLOG("moveReadIndex error, invalid size %d, old_read_index %d, buffer size %d", size, m_read_index, m_buffer.size());  // 记录错误日志
    return;
  }

  m_read_index = j;  // 更新读索引
  adjustBuffer();  // 调整缓冲区
}

void TcpBuffer::moveWriteIndex(int size) {  // 移动写索引
  size_t j = m_write_index + size;  // 计算新的写索引
  if (j >= m_buffer.size()) {  // 如果新的写索引超过缓冲区大小
    ERRORLOG("moveWriteIndex error, invalid size %d, old_read_index %d, buffer size %d", size, m_read_index, m_buffer.size());  // 记录错误日志
    return;
  }

  m_write_index = j;  // 更新写索引
  adjustBuffer();  // 调整缓冲区
}

}  // 命名空间结束
