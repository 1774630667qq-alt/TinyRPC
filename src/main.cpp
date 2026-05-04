/*
 * @Author: Zhang YuHua 1774630667@qq.com
 * @Date: 2026-04-17 17:19:46
 * @LastEditors: Zhang YuHua 1774630667@qq.com
 * @LastEditTime: 2026-04-28 16:17:19
 * @FilePath: /TinyRPC/src/main.cpp
 * @Description: ProtocolUtil 编解码 + 切包循环单元测试
 */
// server_main.cpp
#include "Application.hpp"
#include "Config.hpp"
#include "Logger.hpp"
#include "EventLoop.hpp"
#include "ThreadPool.hpp"
#include "RpcServer.hpp"
#include "rpc/OrderServiceImpl.hpp"

int main() {
    // 1. 一键初始化所有基础设施 (Config → Logger)
    MyRPC::Application::init("conf/tiny_rpc.xml");

    // 3. 初始化底层网络与线程池
    MyRPC::EventLoop loop;
    MyRPC::ThreadPool pool(4); // 4个 Worker 线程
    
    // 4. 初始化 RPC 服务器 (从配置读取端口)
    MyRPC::Config* cfg = MyRPC::Config::GetGlobalConfig();
    MyRPC::RpcServer server(&loop, cfg->server_port_, &pool);

    // 5. 注册服务 (全自动反射获取名字)
    tiny_rpc::OrderService* order_service = new MyRPC::OrderServiceImpl();
    server.registerService(order_service);

    // 6. 启动！
    server.start();
    LOG_INFO << "🚀 RPC Server 已启动，监听 " << cfg->server_port_ << " 端口...";
    loop.loop(); // 死循环，监听 Epoll 事件

    return 0;
}