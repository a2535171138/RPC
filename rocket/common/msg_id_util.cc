#include <fcntl.h>          // 包含文件控制选项的头文件，用于文件操作
#include <unistd.h>         // 包含Unix标准函数定义的头文件，如 read(), open(), close() 等
#include <sys/stat.h>       // 包含文件状态信息的头文件，用于文件操作相关的常量和结构体
#include "rocket/common/msg_id_util.h" // 包含自定义的 MsgIDUtil 工具类的头文件
#include "rocket/common/log.h"          // 包含日志功能的头文件

namespace rocket {

// 定义消息ID的长度为20
static int g_msg_id_length = 20;

// 定义随机数文件描述符，初始值为-1，表示未打开
static int g_random_fd = -1;

// 使用线程局部变量存储当前消息ID号和最大消息ID号
static thread_local string t_msg_id_no;       // 当前消息ID
static thread_local string t_max_msg_id_no;   // 最大消息ID

// 生成唯一消息ID的函数
string MsgIDUtil::GenMsgID() {
    // 如果当前消息ID为空或者已经达到最大值，需要生成新的ID段
    if (t_msg_id_no.empty() || t_msg_id_no == t_max_msg_id_no) {
        // 如果随机数文件描述符未初始化，打开 /dev/urandom 文件
        if (g_random_fd == -1) {
            g_random_fd = open("/dev/urandom", O_RDONLY);
        }

        // 创建一个长度为 g_msg_id_length 的字符串来存储随机数
        string res(g_msg_id_length, 0);

        // 从 /dev/urandom 读取 g_msg_id_length 个字节的随机数据
        if (read(g_random_fd, &res[0], g_msg_id_length) != g_msg_id_length) {
            ERRORLOG("read from /dev/urandom error");  // 读取失败时记录错误日志
            return "";  // 返回空字符串表示失败
        }

        // 将读取到的随机字节转换为数字字符
        for (int i = 0; i < g_msg_id_length; ++i) {
            uint8_t x = ((uint8_t)(res[i])) % 10;  // 将字节值取模10得到0-9的数字
            res[i] = x + '0';  // 转换为字符 '0' 到 '9'
            t_max_msg_id_no += "9";  // 设置最大消息ID号为 "999...9"
        }

        t_msg_id_no = res;  // 将当前消息ID设置为生成的随机数
    } else {
        // 如果当前消息ID尚未达到最大值，则递增消息ID
        size_t i = t_msg_id_no.length() - 1;  // 从ID字符串的最后一个字符开始

        // 从后向前查找第一个不为 '9' 的字符位置
        while (t_msg_id_no[i] == '9' && i >= 0) {
            i--;
        }

        // 如果找到不为 '9' 的字符，则将其递增，并将其后的字符都设置为 '0'
        if (i >= 0) {
            t_msg_id_no[i] += 1;
            for (size_t j = i + 1; j < t_msg_id_no.length(); ++j) {
                t_msg_id_no[j] = '0';
            }
        }
    }

    // 返回生成的消息ID
    return t_msg_id_no;
}

}  // namespace rocket
