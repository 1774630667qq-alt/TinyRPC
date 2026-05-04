#include "Logger.hpp"
#include "AsyncLogging.hpp"
#include "Config.hpp"
#include <sys/time.h>
#include <iostream>

namespace MyRPC {

    // ==========================================================
    // 1. 全局状态
    // ==========================================================
    static AsyncLogging* g_asyncLogger = nullptr;
    static LogLevel g_logLevel = LogLevel::INFO; // 默认输出所有级别

    /**
     * @brief 日志输出模式枚举
     * @details 控制日志消息的输出端点：
     *   - CONSOLE: 仅输出到标准输出 (stdout)
     *   - FILE:    仅写入 AsyncLogging 文件系统
     *   - BOTH:    同时输出到控制台和文件
     */
    enum class AppenderMode { CONSOLE, FILE, BOTH };
    static AppenderMode g_appender = AppenderMode::CONSOLE;

    // ==========================================================
    // 2. 辅助函数
    // ==========================================================

    /**
     * @brief 将配置字符串转换为 LogLevel 枚举
     * @param str 日志级别字符串 (如 "DEBUG", "INFO", "WARNING", "ERROR", "FATAL")
     * @return 对应的 LogLevel 枚举值，无法识别时默认返回 INFO
     */
    static LogLevel stringToLogLevel(const std::string& str) {
        if (str == "DEBUG")   return LogLevel::DEBUG;
        if (str == "INFO")    return LogLevel::INFO;
        if (str == "WARNING") return LogLevel::WARNING;
        if (str == "ERROR")   return LogLevel::ERROR;
        if (str == "FATAL")   return LogLevel::FATAL;
        return LogLevel::INFO; // 无法识别时安全降级
    }

    /**
     * @brief 将配置字符串转换为 AppenderMode 枚举
     * @param str 输出模式字符串 (如 "CONSOLE", "FILE", "BOTH")
     * @return 对应的 AppenderMode 枚举值，无法识别时默认返回 CONSOLE
     */
    static AppenderMode stringToAppender(const std::string& str) {
        if (str == "FILE") return AppenderMode::FILE;
        if (str == "BOTH") return AppenderMode::BOTH;
        return AppenderMode::CONSOLE; // 无法识别时安全降级
    }

    // ==========================================================
    // 3. 初始化函数（自动从 Config 读取所有参数）
    // ==========================================================
    void initGlobalLogger() {
        Config* cfg = Config::GetGlobalConfig();
        if (!cfg) {
            std::cerr << "[Logger] WARN: Config 尚未初始化，使用默认配置 (CONSOLE + INFO)" << std::endl;
            return;
        }

        // 设置日志级别
        setLogLevel(stringToLogLevel(cfg->log_level_));

        // 设置输出模式
        g_appender = stringToAppender(cfg->log_appender_);

        // 如果输出模式包含 FILE，创建并启动 AsyncLogging
        if (g_appender == AppenderMode::FILE || g_appender == AppenderMode::BOTH) {
            std::string basename = cfg->log_file_path_ + "/" + cfg->log_file_name_;
            g_asyncLogger = new AsyncLogging(basename);
            g_asyncLogger->start(); // 启动后台写日志线程
        }
    }

    void destroyGlobalLogger() {
        if (g_asyncLogger) {
            g_asyncLogger->stop();   // 唤醒后台线程 → 刷盘 → join
            delete g_asyncLogger;
            g_asyncLogger = nullptr;
        }
        // 重置为 CONSOLE 模式，这样 destroy 后如果还有日志输出也不会崩
        g_appender = AppenderMode::CONSOLE;
    }

    void setLogLevel(LogLevel level) { g_logLevel = level; }

    LogLevel getLogLevel() { return g_logLevel; }

    // ==========================================================
    // 4. 辅助数组：把枚举转换成字符串前缀
    // ==========================================================
    const char* LogLevelName[5] = {
        "[DEBUG]",
        "[INFO] ",
        "[WARN] ",
        "[ERROR]",
        "[FATAL]"
    };

    Logger::Logger(const char* file, int line, LogLevel level)
        : level_(level), file_(file), line_(line) {
        
        // 1. 打印时间戳 (我已为你写好这个比较繁琐的系统调用)
        formatTime();

        // 2. 打印日志级别 (利用 level_ 作为索引，去 LogLevelName 数组里拿前缀)
        stream_ << LogLevelName[static_cast<int>(level_)];

        // 3. 打印线程 ID (在高并发下，知道是哪个线程打印的极其重要)
        // 提示：std::this_thread::get_id() 返回的是个对象，你可以把它转成简单的 hash 整数或者直接忽略，为了简单，先追加一个常量字符串如 "[TID] " 占位。
        // 为了简单和高性能，暂时不格式化 std::thread::id，直接用占位符
        stream_ << "[TID] "; // 实际项目中可以缓存哈希值
    }

    Logger::~Logger() {
        // 1. 日志的正文其实在构造到析构之间，已经被业务层通过 << 追加到 stream_ 里了！
        // 2. 打印文件名和行号，并加上换行符！
        stream_ << " - " << file_ << ':' << line_ << '\n';

        const LogStream::Buffer& buf = stream_.buffer();

        // 3. 根据输出模式，将日志发送到对应的输出端
        if (g_appender == AppenderMode::CONSOLE || g_appender == AppenderMode::BOTH) {
            // 输出到控制台
            std::cout.write(buf.data(), buf.length());
            std::cout.flush();
        }

        if (g_appender == AppenderMode::FILE || g_appender == AppenderMode::BOTH) {
            // 写入异步日志文件
            if (g_asyncLogger) {
                g_asyncLogger->append(buf.data(), buf.length());
            }
        }

        // Fallback: 如果 g_asyncLogger 为空且模式不含 CONSOLE，至少保底输出到屏幕
        if (!g_asyncLogger && g_appender != AppenderMode::CONSOLE && g_appender != AppenderMode::BOTH) {
            std::cout.write(buf.data(), buf.length());
            std::cout.flush();
        }

        // 4. 如果是 FATAL 级别的错误，写完日志后直接让整个程序崩溃退出 (保命)
        if (level_ == LogLevel::FATAL) {
            abort();
        }
    }

    void Logger::formatTime() {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        time_t seconds = tv.tv_sec;
        struct tm tm_time;
        localtime_r(&seconds, &tm_time);

        char buf[64];
        // 格式化为：2026-03-27 15:30:00.123456 
        int len = snprintf(buf, sizeof(buf), "%4d-%02d-%02d %02d:%02d:%02d.%06ld ",
            tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
            tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
            tv.tv_usec);
        
        stream_.append(buf, len);
    }

} // namespace MyRPC