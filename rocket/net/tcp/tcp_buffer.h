#ifndef ROCKET_NET_TCP_TCP_BUFFER_H
#define ROCKET_NET_TCP_TCP_BUFFER_H

#include <vector>  // 引入标准库中的vector头文件

using namespace std;  // 使用标准命名空间

namespace rocket {  // 定义命名空间rocket

class TcpBuffer {  // 定义TcpBuffer类
  public:
    typedef std::shared_ptr<TcpBuffer> s_ptr;

    TcpBuffer(int size);  // 构造函数，初始化缓冲区大小

    ~TcpBuffer();  // 析构函数，释放资源

    int readAble();  // 返回当前可读数据的大小

    int writeAble();  // 返回当前可写空间的大小

    int readIndex();  // 返回当前读索引

    int writeIndex();  // 返回当前写索引

    void writeToBuffer(const char* buf, int size);  // 将数据写入缓冲区

    void readFromBuffer(vector<char>& re, int size);  // 从缓冲区读取数据

    void resizeBuffer(int new_size);  // 调整缓冲区大小

    void adjustBuffer();  // 调整缓冲区，使未读数据移到缓冲区起始位置

    void moveReadIndex(int size);  // 移动读索引

    void moveWriteIndex(int size);  // 移动写索引

    vector<char> m_buffer;  // 缓冲区，使用vector<char>实现

  private:
    int m_read_index {0};  // 读索引，初始值为0
    int m_write_index {0};  // 写索引，初始值为0
    int m_size {0};  // 缓冲区大小，初始值为0

};

}

#endif  // 结束预处理指令，防止重复包含
