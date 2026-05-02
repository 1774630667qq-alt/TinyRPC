#pragma once
#include <google/protobuf/service.h>
#include <string>

namespace MyRPC {

/**
 * @brief RPC 控制器：记录一次 RPC 调用的状态信息
 *
 * 继承自 google::protobuf::RpcController，在客户端和服务端两侧都可以使用。
 *
 * 客户端侧：
 * - 调用前 Reset() 清空状态
 * - 调用后通过 Failed() 检查是否出错，ErrorText() 获取错误描述
 *
 * 服务端侧：
 * - 业务处理中通过 SetFailed() 标记错误，框架会自动发送错误响应
 *
 * 生命周期：
 * - 由调用方在栈上或堆上分配，生命周期覆盖整个 RPC 调用过程
 * - RpcChannel 在收到回包后会通过控制器指针设置错误信息
 */
class RpcController : public google::protobuf::RpcController {
public:
    RpcController() = default;
    ~RpcController() override = default;

    // ==================== 客户端侧接口 ====================

    /**
     * @brief 重置控制器状态（在发起新的 RPC 调用前调用）
     */
    void Reset() override {
        failed_ = false;
        errorText_.clear();
        canceled_ = false;
        errCode_ = 0;
    }

    /**
     * @brief 检查 RPC 调用是否失败
     * @return true 表示调用失败（服务端返回错误或网络异常）
     */
    bool Failed() const override {
        return failed_;
    }

    /**
     * @brief 获取错误描述信息
     * @return 错误字符串，如果未失败则为空
     */
    std::string ErrorText() const override {
        return errorText_;
    }

    /**
     * @brief 发起取消请求（当前未实现）
     */
    void StartCancel() override {
        canceled_ = true;
    }

    // ==================== 服务端侧接口 ====================

    /**
     * @brief 标记本次 RPC 调用失败，并设置错误信息
     * @param reason 错误描述
     */
    void SetFailed(const std::string& reason) override {
        failed_ = true;
        errorText_ = reason;
    }

    /**
     * @brief 检查客户端是否已取消此次调用（当前未实现）
     * @return true 表示已取消
     */
    bool IsCanceled() const override {
        return canceled_;
    }

    /**
     * @brief 注册取消通知回调（当前未实现）
     */
    void NotifyOnCancel(google::protobuf::Closure* callback) override {
        // TODO: 未来可在此注册取消时的清理回调
    }

    // ==================== 扩展接口 ====================

    /**
     * @brief 设置错误码（配合 RpcMeta.err_code 使用）
     * @param code 整型错误码
     */
    void SetErrorCode(int32_t code) { errCode_ = code; }

    /**
     * @brief 获取错误码
     */
    int32_t ErrorCode() const { return errCode_; }

private:
    bool failed_ = false;        ///< 是否失败
    std::string errorText_;       ///< 错误描述
    bool canceled_ = false;       ///< 是否已取消
    int32_t errCode_ = 0;        ///< 错误码
};

} // namespace MyRPC
