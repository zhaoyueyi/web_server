//
// Created by 98302 on 2023/9/20.
//

#ifndef WEB_SERVER_LOG_H
#define WEB_SERVER_LOG_H

#include <memory>
#include <cstdarg>
#include <sys/stat.h>
#include "../buffer/Buffer.h"
#include "block_queue.h"

/*
 * 日志类
 * 单例模式
 * 异步：日志先存入阻塞队列，写线程读取队列写入日志
 * 读线程支持多线程写入队列，写线程单线程读取队列写入文件
 */
class Log {
public:
    void Init(int level,
              const char *path = "./log",
              const char *suffix = ".log",
              int max_queue_capacity = 1024);

    static Log *Instance();

    static void FlushLogThread();

    void Write(int level, const char *format, ...);

    void Flush();

    int GetLevel();

    void SetLevel(int level);

    bool IsOpen() const { return is_open_; }

private:
    Log();

    void AppendLogLevelTitle_(int level);

    virtual ~Log();

    void AsyncWrite_();

    static const int LOG_PATH_LEN = 256;
    static const int LOG_NAME_LEN = 256;
    static const int MAX_LINES = 50000;

    const char *path_;
    const char *suffix_;

    int MAX_LINES_;
    int line_count_;
    int today_;

    bool is_open_;
    Buffer buffer_;  // 单次日志缓冲区
    int level_;
    bool is_async_;

    FILE *fp_;
    std::unique_ptr<BlockQueue<std::string>> deque_;  // 异步日志缓存队列
    std::unique_ptr<std::thread> write_thread_;  // 写线程
    std::mutex mtx_;  // 访问fp加锁
};

/*
 * 通过宏定义外部使用接口
 * ...为可变参数，对应##__VA_ARGS__替换
 */
#define LOG_BASE(level, format, ...) \
        {                            \
            Log* log = Log::Instance();  \
            if(log->IsOpen() && log->GetLevel() <= level){ \
                log->Write(level, format, ##__VA_ARGS__);  \
                log->Flush();            \
            }                            \
        }                            \

#define LOG_DEBUG(format, ...) LOG_BASE(0, format, ##__VA_ARGS__);
#define LOG_INFO(format, ...) LOG_BASE(1, format, ##__VA_ARGS__);
#define LOG_WARN(format, ...) LOG_BASE(2, format, ##__VA_ARGS__);
#define LOG_ERROR(format, ...) LOG_BASE(3, format, ##__VA_ARGS__);
#endif //WEB_SERVER_LOG_H
