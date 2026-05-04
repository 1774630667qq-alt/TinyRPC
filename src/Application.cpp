// src/Application.cpp
#include "Application.hpp"
#include "Config.hpp"
#include "Logger.hpp"

namespace MyRPC {

void Application::init(const char* config_path) {
    // ① 配置系统（无依赖，必须最先初始化）
    Config::SetGlobalConfig(config_path);

    // ② 日志系统（依赖 Config 中的 log_level_, log_appender_, log_file_path_ 等）
    initGlobalLogger();

    // ③ 未来扩展点：
    // initZooKeeper();
    // installSignalHandlers();

    LOG_INFO << "TinyRPC 基础设施初始化完毕 ✅";
}

} // namespace MyRPC
