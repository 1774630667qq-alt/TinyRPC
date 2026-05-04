/**
 * @brief RPC 客户端测试：同时展示异步和同步两种调用模式
 *
 * 模式 1（异步）：
 *   通过 done 回调在 EventLoop 线程中处理响应
 *
 * 模式 2（同步）：
 *   利用 std::promise + std::future 阻塞等待异步回包，
 *   实现"看起来同步"的 RPC 调用体验
 *
 * 两种模式共用同一个 TcpClient 长连接和 EventLoop。
 */
#include "Application.hpp"
#include "EventLoop.hpp"
#include "TcpClient.hpp"
#include "TcpConnection.hpp"
#include "rpc/RpcChannel.hpp"
#include "rpc/RpcController.hpp"
#include "Logger.hpp"
#include "ModernClosure.hpp"
#include "order.pb.h"
#include <future>
#include <thread>

int main() {
    // 1. 一键初始化所有基础设施 (Config → Logger)
    MyRPC::Application::init("conf/tiny_rpc.xml");

    MyRPC::EventLoop loop;
    MyRPC::TcpClient client(&loop, "127.0.0.1", 8080);
    MyRPC::RpcChannel channel(&client, &loop);

    // 注册 RpcChannel::onMessage 为 TcpClient 的消息回调
    client.setMessageCallback(
        [&channel](const std::shared_ptr<MyRPC::TcpConnection>& conn, MyRPC::Buffer* buf) {
            channel.onMessage(conn, buf);
        }
    );

    // 连接建立后，在独立线程中执行同步 RPC 调用
    client.setConnectionCallback(
        [&channel, &loop](const std::shared_ptr<MyRPC::TcpConnection>& conn) {
            LOG_INFO << "✅ 已连接到服务器";

            // =====================================================
            // 在独立线程中执行同步调用（不能阻塞 EventLoop 线程！）
            // =====================================================
            std::thread([&channel, &loop]() {
                LOG_INFO << "📡 [同步模式] 开始发起 RPC 调用...";

                // 1. 准备请求/响应/控制器
                tiny_rpc::OrderRequest req;
                tiny_rpc::OrderResponse resp;
                MyRPC::RpcController controller;
                req.set_order_id("ORD-SYNC-114514");

                // 2. 用 promise/future 把异步回调变成同步等待
                std::promise<void> promise;
                std::future<void> future = promise.get_future();

                auto* done = new MyRPC::ModernClosure(
                    [&promise]() {
                        promise.set_value(); // 唤醒阻塞的 future.get()
                    }
                );

                // 3. 发起 RPC 调用（立即返回，异步发送）
                tiny_rpc::OrderService_Stub stub(&channel);
                stub.QueryOrder(&controller, &req, &resp, done);
                LOG_INFO << "📤 [同步模式] 请求已发送，阻塞等待回包...";

                // 4. 阻塞等待直到 done 回调执行
                future.get();

                // 5. 检查调用结果
                if (controller.Failed()) {
                    LOG_ERROR << "❌ [同步模式] RPC 调用失败!"
                              << " 错误码: " << controller.ErrorCode()
                              << " 错误信息: " << controller.ErrorText();
                } else {
                    LOG_INFO << "\n🎉🎉🎉 [同步模式] RPC 调用成功！";
                    LOG_INFO << "📥 RetCode: " << resp.ret_code()
                             << ", Info: " << resp.res_info()
                             << ", OrderID: " << resp.order_id();
                }

                // 退出事件循环
                loop.quit();
            }).detach();
        }
    );

    LOG_INFO << "🚀 启动 RPC 客户端 (同步调用 Demo)，连接 127.0.0.1:8080 ...";
    client.connect();
    loop.loop();  // 事件循环驱动底层网络 IO

    LOG_INFO << "👋 客户端退出";

    // 优雅退出：刷盘日志 + 释放全局资源
    MyRPC::Application::destroy();
    return 0;
}
