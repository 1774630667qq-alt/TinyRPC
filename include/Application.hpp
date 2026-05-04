// include/Application.hpp
#pragma once

namespace MyRPC {

/**
 * @brief 框架启动引导器 (Bootstrap)
 *
 * 负责管理 TinyRPC 基础设施的完整生命周期：初始化(init) 和 销毁(destroy)。
 *
 * 初始化阶段（init）：按正确的依赖顺序启动子系统
 *   1. Config  — 解析 XML 配置文件
 *   2. Logger  — 根据 Config 中的参数初始化日志级别和输出模式
 *   (未来可扩展：ZooKeeper 注册中心、信号处理器 等)
 *
 * 销毁阶段（destroy）：按初始化的逆序安全关闭子系统
 *   1. Logger  — 停止异步写线程，刷盘所有缓冲区
 *   2. Config  — 释放配置单例的堆内存
 *
 * 设计哲学：
 *   - Application 只管"基础设施编排"，不管理 EventLoop、ThreadPool 等运行时对象
 *   - 所有入口文件（server / client / test）只需 init() + destroy() 即可管理完整生命周期
 *   - 新增子系统时，只需修改 Application，无需改动每个入口
 *
 * @code
 * int main() {
 *     MyRPC::Application::init("conf/tiny_rpc.xml");
 *     // 业务逻辑...
 *     MyRPC::Application::destroy();
 *     return 0;
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

    /**
     * @brief 销毁所有基础设施子系统，安全释放资源
     *
     * 内部按初始化的逆序销毁：
     *   ① destroyGlobalLogger()      — 停止 AsyncLogging 后台线程，刷盘剩余缓冲区
     *   ② Config::DestroyGlobalConfig() — 释放 Config 单例堆内存
     *
     * @note 必须在进程退出前调用，否则异步日志缓冲区中的最后几秒日志可能丢失
     */
    static void destroy();

private:
    // 禁止实例化，这是一个纯静态工具类
    Application() = delete;
    ~Application() = delete;
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
};

} // namespace MyRPC
