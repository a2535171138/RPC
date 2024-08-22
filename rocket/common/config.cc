#include <tinyxml/tinyxml.h>
#include "rocket/common/config.h"

// 定义宏：读取XML节点，检查节点是否存在
#define READ_XML_NODE(name, parent) \
TiXmlElement* name##_node = parent->FirstChildElement(#name); \
if (!name##_node) { \
    printf("Start rocket server error, failed to read node [%s]\n", #name); \
    exit(0); \
}

// 定义宏：从XML节点中读取字符串，检查节点及其文本是否存在
#define READ_STR_FROM_XML_NODE(name, parent) \
TiXmlElement* name##_node = parent->FirstChildElement(#name); \
if (!name##_node || !name##_node->GetText()) { \
    printf("Start rocket server error, failed to read config file %s\n", #name); \
    exit(0); \
} \
std::string name##_str = std::string(name##_node->GetText());


namespace rocket{

static Config* g_config = NULL; // 静态全局配置指针

// 获取全局配置实例
Config* Config::GetGlobalConfig(){
  return g_config;  // 返回全局配置实例
}

// 设置全局配置实例
void Config::SetGlobalConfig(const char* xmlfile){
  if(g_config == NULL){  // 如果全局配置为空
    g_config = new Config(xmlfile);  // 创建新的配置实例
  }
}

// 构造函数，从指定的 XML 文件加载配置
Config::Config(const char* xmlfile){
  TiXmlDocument *xml_document = new TiXmlDocument();  // 创建XML文档对象
  bool rt = xml_document->LoadFile(xmlfile);  // 加载指定的XML文件

  // 如果加载失败，打印加载错误信息并退出
  if(!rt){  
    printf("Start rocket server error, failed to read config file %s, error info[%s] \n", xmlfile, xml_document->ErrorDesc());
    exit(0);
  }

  READ_XML_NODE(root, xml_document);  // 读取根节点
  READ_XML_NODE(log, root_node);  // 读取日志节点

  READ_STR_FROM_XML_NODE(log_level, log_node);  // 读取日志级别字符串
  READ_STR_FROM_XML_NODE(log_file_name, log_node);
  READ_STR_FROM_XML_NODE(log_file_path, log_node);
  READ_STR_FROM_XML_NODE(log_max_file_size, log_node);
  READ_STR_FROM_XML_NODE(log_sync_interval, log_node);

  m_log_level = log_level_str;  // 将读取的日志级别字符串赋值给成员变量
  m_log_file_name = log_file_name_str;
  m_log_file_path = log_file_path_str;
  m_log_max_file_size = atoi(log_max_file_size_str.c_str()); // 将字符串转换为整数
  m_log_sync_interval = atoi(log_sync_interval_str.c_str()); // 将字符串转换为整数

  // 打印日志配置信息
  printf("LOG -- CONFIG LEVEL[%s], FILE_NAME[%s], FILE_PATH[%s], MAX_FILE_SIZE[%d B], SYNC_INTEVAL[%d ms]\n",
    m_log_level.c_str(), m_log_file_name.c_str(), m_log_file_path.c_str(), m_log_max_file_size, m_log_sync_interval);

}

}
