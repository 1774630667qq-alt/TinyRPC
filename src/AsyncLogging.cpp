/*
 * @Author: Zhang YuHua 1774630667@qq.com
 * @Date: 2026-03-27 17:15:20
 * @LastEditors: Zhang YuHua 1774630667@qq.com
 * @LastEditTime: 2026-03-27 17:55:08
 * @FilePath: /ServerPractice/src/AsyncLogging.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "AsyncLogging.hpp"
#include <iostream>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <sys/stat.h>
#include <sys/types.h>

namespace MyRPC {

    /**
     * @brief 根据 basename 生成带时间戳的日志文件路径
     * @param basename 日志文件基础路径（如 "./log/tiny_rpc_server"）
     * @return 带时间戳的完整路径（如 "./log/tiny_rpc_server_20260504_133800.log"）
     *
     * 文件名格式: {basename}_{YYYYMMDD}_{HHMMSS}.log
     * 这样每次进程启动都会生成独立的日志文件，便于排查问题和日志归档。
     */
    static std::string generateLogFileName(const std::string& basename) {
        time_t now = time(nullptr);
        struct tm tm_time;
        localtime_r(&now, &tm_time);

        char timestamp[64];
        snprintf(timestamp, sizeof(timestamp), "_%04d%02d%02d_%02d%02d%02d.log",
            tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
            tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);

        return basename + timestamp;
    }

    /**
     * @brief 确保日志文件所在的目录存在
     * @param filepath 完整的日志文件路径
     *
     * 从路径中提取目录部分，如果目录不存在则创建（权限 0755）。
     */
    static void ensureDirectoryExists(const std::string& filepath) {
        size_t pos = filepath.find_last_of('/');
        if (pos != std::string::npos) {
            std::string dir = filepath.substr(0, pos);
            mkdir(dir.c_str(), 0755); // 忽略已存在的错误
        }
    }

    AsyncLogging::AsyncLogging(const std::string& basename)
        : basename_(basename),
          running_(false),
          currentBuffer_(new Buffer),
          nextBuffer_(new Buffer) {
        currentBuffer_->bzero();
        nextBuffer_->bzero();
        buffers_.reserve(16); // 预分配一点空间
    }

    AsyncLogging::~AsyncLogging() {
        if (running_) {
            stop();
        }
    }

    void AsyncLogging::append(const char* logline, int len) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (currentBuffer_->avail() >= len) {
                currentBuffer_->append(logline, len);
            } else {
                buffers_.push_back(std::move(currentBuffer_));
                if (nextBuffer_) {
                    currentBuffer_ = std::move(nextBuffer_);
                } else {
                    currentBuffer_.reset(new Buffer);
                }
                currentBuffer_->append(logline, len);
                cond_.notify_one();
            }
        }
    }

    void AsyncLogging::threadFunc() {
        // 1. 生成带时间戳的日志文件名，并确保目录存在
        std::string logFileName = generateLogFileName(basename_);
        ensureDirectoryExists(logFileName);

        FILE* fp = fopen(logFileName.c_str(), "ae"); // "ae" = append + close-on-exec
        if (!fp) {
            std::cerr << "[AsyncLogging] ERROR: 无法打开日志文件: " << logFileName << std::endl;
            return;
        }

        // 2. 在后台线程里，准备两个新的"空桶"。这两个空桶专门用来和前台交换！
        BufferPtr newBuffer1(new Buffer);
        BufferPtr newBuffer2(new Buffer);
        newBuffer1->bzero();
        newBuffer2->bzero();

        // 3. 准备一个后台专用的队列，用来接收前台装满的桶
        BufferVector buffersToWrite;
        buffersToWrite.reserve(16);

        while (running_) {
            {
                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    cond_.wait_for(lock, std::chrono::seconds(3), [this] { return !buffers_.empty() || !running_; });
                    buffers_.push_back(std::move(currentBuffer_));
                    currentBuffer_ = std::move(newBuffer1);
                    buffersToWrite.swap(buffers_);
                    if (!nextBuffer_) {
                        nextBuffer_ = std::move(newBuffer2);
                    }
                }
            }
            // 防御性编程：如果瞬间涌入海量日志（比如 buffersToWrite 超过了 25 个），说明磁盘完全跟不上了。
            // 工业级做法：直接砍掉多余的日志，只保留前两个（或者直接清空过多的部分），保命要紧！
            if (buffersToWrite.size() > 25) {
                buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
            }

            // 遍历 buffersToWrite 里的所有满桶，把数据写入日志文件
            for (const auto& buffer : buffersToWrite) {
                fwrite(buffer->data(), 1, buffer->length(), fp);
            }
            fflush(fp); // 每轮写完后刷盘，确保数据落地

            // 重新回收备用桶！
            if (!newBuffer1) {
                newBuffer1 = std::move(buffersToWrite.back());
                buffersToWrite.pop_back();
                newBuffer1->reset();
            }
            if (!newBuffer2) {
                newBuffer2 = std::move(buffersToWrite.back());
                buffersToWrite.pop_back();
                newBuffer2->reset();
            }

            // 清空 buffersToWrite，开启下一轮死循环！
            buffersToWrite.clear();
        }

        // 线程退出前最后一次刷盘
        fflush(fp);
        fclose(fp);
    }
} // namespace MyRPC