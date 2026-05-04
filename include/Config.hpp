/*
 * @Author: Zhang YuHua 1774630667@qq.com
 * @Date: 2026-05-04 11:57:59
 * @LastEditors: Zhang YuHua 1774630667@qq.com
 * @LastEditTime: 2026-05-04 12:08:28
 * @FilePath: /TinyRPC/include/Config.hpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置
 * 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
// include/Config.hpp
#pragma once
#include <string>

namespace MyRPC {

/**
 * @brief 全局配置管理类 (单例模式)
 * * 负责在系统启动时加载配置文件，并为整个框架的运行周期内提供统一的配置参数访问接口。
 */
class Config {
public:
    /**
     * @brief 获取全局配置单例对象
     * * @return Config* 返回配置对象的指针。
     * @note 在调用此方法前，必须先调用 SetGlobalConfig 进行初始化，否则可能会引发空指针异常或未定义的行为。
     */
    static Config* GetGlobalConfig();

    /**
     * @brief 初始化全局配置单例
     * * @param xmlfile 配置文件的路径（例如 "conf/tiny_rpc.xml"）
     * @note 整个进程生命周期内只应在 main 函数的最开始调用一次。
     */
    static void SetGlobalConfig(const char* xmlfile);

public:
    // ==========================================
    // 配置项参数区 (可通过 GetGlobalConfig()->xxx 访问)
    // ==========================================

    /* 日志相关配置 */
    std::string log_level_;     ///< 日志级别 (如 "DEBUG", "INFO", "ERROR")
    std::string log_file_name_; ///< 日志文件名前缀 (如 "tiny_rpc_server")
    std::string log_file_path_; ///< 日志文件存放的目录路径 (如 "./log")
    std::string log_appender_;  ///< 日志输出模式 ("CONSOLE" / "FILE" / "BOTH")
    
    /* RPC 服务端网络配置 */
    std::string server_ip_;     ///< 当前 RPC 服务器绑定的 IP 地址 (如 "127.0.0.1" 或 "0.0.0.0")
    int server_port_;           ///< 当前 RPC 服务器监听的端口号 (如 8081)
    
    /* ZooKeeper 注册中心配置 (预留给微服务治理架构) */
    std::string zk_ip_;         ///< ZooKeeper 节点的 IP 地址
    int zk_port_;               ///< ZooKeeper 节点的端口号 (默认通常是 2181)

private:
    /**
     * @brief 默认构造函数（私有化以保证单例模式）
     */
    Config();

    /**
     * @brief 带参构造函数
     * @param xmlfile 配置文件路径
     */
    Config(const char* xmlfile);

    /**
     * @brief 析构函数
     */
    ~Config();

    // ==========================================
    // 禁用拷贝语义，彻底堵死产生多个实例的可能
    // ==========================================
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    /**
     * @brief 解析配置文件的核心实现
     * * 内部调用具体的解析库（如 tinyxml2），将 XML 节点的值提取并赋给类的成员变量。
     * @param xmlfile 配置文件路径
     */
    void readConf(const char* xmlfile);

private:
    static Config* global_config_; ///< 全局唯一的单例对象指针
};

} // namespace MyRPC