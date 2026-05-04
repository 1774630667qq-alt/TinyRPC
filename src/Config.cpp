/*
 * @Author: Zhang YuHua 1774630667@qq.com
 * @Date: 2026-05-04 11:58:13
 * @LastEditors: Zhang YuHua 1774630667@qq.com
 * @LastEditTime: 2026-05-04 13:07:53
 * @FilePath: /TinyRPC/src/Config.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置
 * 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "Config.hpp"
#include "tinyxml2.h"
#include "Logger.hpp"

#include <cstdlib>

namespace MyRPC {
Config *Config::global_config_ = nullptr;

Config *Config::GetGlobalConfig() {
    return global_config_;
}

void Config::SetGlobalConfig(const char *xmlfile) {
    if (global_config_ == nullptr) {
        global_config_ = new Config(xmlfile);
    } else {
        global_config_->readConf(xmlfile);
    }
}

void Config::DestroyGlobalConfig() {
    delete global_config_;
    global_config_ = nullptr;
}

Config::Config() : server_port_(0), zk_port_(0) {}

Config::Config(const char* xmlfile) : server_port_(0), zk_port_(0) {
    readConf(xmlfile);
}

Config::~Config() {}

/**
 * @brief 安全获取 XML 元素的文本内容
 * @param parent 父节点
 * @param tag 子标签名
 * @param default_val 当标签不存在或内容为空时的默认值
 * @return 解析到的文本或默认值
 */
static std::string getTextSafe(tinyxml2::XMLElement *parent,
                               const char *tag,
                               const char *default_val) {
    tinyxml2::XMLElement *node = parent->FirstChildElement(tag);
    if (node && node->GetText()) {
        return node->GetText();
    }
    return default_val;
}

/**
 * @brief 安全获取 XML 元素的整数内容
 * @param parent 父节点
 * @param tag 子标签名
 * @param default_val 当标签不存在或内容为空时的默认值
 * @return 解析到的整数或默认值
 */
static int getIntSafe(tinyxml2::XMLElement *parent,
                      const char *tag,
                      int default_val) {
    tinyxml2::XMLElement *node = parent->FirstChildElement(tag);
    if (node && node->GetText()) {
        return std::atoi(node->GetText());
    }
    return default_val;
}

void Config::readConf(const char* xmlfile) {
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError error = doc.LoadFile(xmlfile);
    if (error != tinyxml2::XML_SUCCESS) {
        LOG_ERROR << "LoadFile fail, file path: " << xmlfile << " error: " << doc.ErrorStr();
        return;
    }

    tinyxml2::XMLElement *root = doc.RootElement();
    if (!root) {
        LOG_ERROR << "Root element not found";
        return;
    }

    // 解析日志标签
    tinyxml2::XMLElement *log_node = root->FirstChildElement("log");
    if (!log_node) {
        LOG_WARNING << "缺少 log 标签，使用默认日志配置";
        log_level_ = "INFO";
        log_file_name_ = "tiny_rpc_server";
        log_file_path_ = "./log";
    } else {
        log_level_     = getTextSafe(log_node, "log_level", "INFO");
        log_file_name_ = getTextSafe(log_node, "log_file_name", "tiny_rpc_server");
        log_file_path_ = getTextSafe(log_node, "log_file_path", "./log");
        log_appender_  = getTextSafe(log_node, "log_appender", "CONSOLE");
    }

    // 解析 server 标签
    tinyxml2::XMLElement *server_node = root->FirstChildElement("server");
    if (!server_node) {
        LOG_WARNING << "缺少 server 标签，使用默认 server 配置";
        server_ip_ = "127.0.0.1";
        server_port_ = 8081;
    } else {
        server_ip_   = getTextSafe(server_node, "ip", "127.0.0.1");
        server_port_ = getIntSafe(server_node, "port", 8081);
    }

    // 解析 zookeeper 标签
    tinyxml2::XMLElement *zookeeper_node = root->FirstChildElement("zookeeper");
    if (!zookeeper_node) {
        LOG_WARNING << "缺少 zookeeper 标签，使用默认 zookeeper 配置";
        zk_ip_ = "127.0.0.1";
        zk_port_ = 2181;
    } else {
        zk_ip_   = getTextSafe(zookeeper_node, "ip", "127.0.0.1");
        zk_port_ = getIntSafe(zookeeper_node, "port", 2181);
    }
}
} // namespace MyRPC