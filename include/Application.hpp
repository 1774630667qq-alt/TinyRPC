// include/Application.hpp
#pragma once

namespace MyRPC {

/**
 * @brief 框架启动引导器 (Bootstrap)
 *
 * 负责在进程启动时，按正确的依赖顺序初始化所有基础设施子系统：
 *   1. Config  — 解析 XML 配置文件
 *   2. Logger  — 根据 Config 中的参数初始化日志级别和输出模式
 *   (未来可扩展：ZooKeeper 注册中心、信号处理器 等)
 *
 * 设计哲学：
 *   - Application 只管"基础设施编排"，不管理 EventLoop、ThreadPool 等运行时对象
 *   - 所有入口文件（server / client / test）只需一行 Application::init() 即可完成启动
 *   - 新增子系统时，只需修改 Application::init()，无需改动每个入口
 *
 * @code
 * int main() {
 *     MyRPC::Application::init("conf/tiny_rpc.xml");
 *     // 后续业务逻辑...
 * }
 * @endcode
 */
class Application {
public:
    /**
     * @brief 初始化所有基础设施子系统
     * @param config_path XML 配置文件路径（如 "conf/tiny_rpc.xml"）
     *
     * 内部按固定的依赖顺序初始化：
     *   ① Config::SetGlobalConfig()  — 加载配置（无依赖，必须最先）
     *   ② initGlobalLogger()         — 日志系统（依赖 Config）
     *   ③ (预留扩展点)
     */
    static void init(const char* config_path);

private:
    // 禁止实例化，这是一个纯静态工具类
    Application() = delete;
    ~Application() = delete;
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
};

} // namespace MyRPC
