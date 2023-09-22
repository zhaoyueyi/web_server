//
// Created by 98302 on 2023/9/20.
//


#include "Log.h"

void Log::Init(int level, const char *path, const char *suffix, int max_queue_capacity) {
    is_open_ = true;
    level_ = level;
    path_ = path;
    suffix_ = suffix;
    if(max_queue_capacity > 0){  // 异步，使用BlockQueue
        is_async_ = true;
        if(!deque_) {  // 智能指针内容为空则创建
//            unique_ptr<BlockQueue<std::string>> new_queue(new BlockQueue<std::string>);
//            deque_ = std::move(new_queue);
            deque_ = make_unique<BlockQueue<std::string>>();
//            unique_ptr<thread> new_thread(new thread(FlushLogThread));
//            write_thread_ = std::move(new_thread);
            write_thread_ = make_unique<thread>(FlushLogThread);  // 创建写线程，一直执行Flush
        }
    }else{  // 同步
        is_async_ = false;
    }
    line_count_ = 0;
    time_t timer = time(nullptr);
    struct tm* systime = localtime(&timer);
    char file_name[LOG_NAME_LEN] = {0};
    snprintf(file_name,
             LOG_NAME_LEN-1,
             "%s/%04d_%02d_%02d%s",
             path_,
             systime->tm_year+1900,
             systime->tm_mon+1,
             systime->tm_mday,
             suffix_);  // 格式化输出字符串
    today_ = systime->tm_mday;

    // 修改fp，创建lock_guard作用域
    {
        lock_guard<mutex> locker(mtx_);
        buffer_.RetrieveAll();
        if(fp_){  // fp已打开，重新打开
            Flush();
            fclose(fp_);
        }
        fp_ = fopen(file_name, "a");  // 追加模式打开文件
        if(fp_ == nullptr){
            mkdir(file_name, 0777);  // 创建文件
            fp_ = fopen(file_name, "a");
        }
        assert(fp_ != nullptr);
    }
}

/// 获取单例日志对象
/// \return
Log *Log::Instance() {
    static Log log;  // 懒汉模式，请求实例时初始化
    return &log;
}

/// 异步日志写线程
void Log::FlushLogThread() {
    Log::Instance()->AsyncWrite_();
}

/// 写入日志
/// \param level
/// \param format
/// \param ...
void Log::Write(int level, const char *format, ...) {
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t t_sec = now.tv_sec;
    struct tm *sys_time = localtime(&t_sec);
    struct tm t = *sys_time;
    va_list vaList;

    // 运行时日期更新 | 当前日志行数已满 => 新建文件记录
    if (today_ != t.tm_mday
        || (line_count_ && (line_count_ % MAX_LINES == 0))) {
        unique_lock<mutex> locker(mtx_);
        locker.unlock();

        char new_file[LOG_NAME_LEN];
        char tail[36] = {0};
        snprintf(tail,
                 36,
                 "%04d_%02d_%02d",
                 t.tm_year+1900,
                 t.tm_mon+1,
                 t.tm_mday);
        if(today_ != t.tm_mday){
            snprintf(new_file,
                     LOG_NAME_LEN-72,
                     "%s/%s%s",
                     path_,
                     tail,
                     suffix_);
            today_ = t.tm_mday;
            line_count_ = 0;
        }else{
            snprintf(new_file,
                     LOG_NAME_LEN-72,
                     "%s/%s-%d%s",
                     path_,
                     tail,
                     (line_count_/MAX_LINES),
                     suffix_);
        }

        locker.lock();
        Flush();
        fclose(fp_);
        fp_ = fopen(new_file, "a");
        assert(fp_ != nullptr);
    }
    // 写入日志
    {
        unique_lock<mutex> locker(mtx_);
        line_count_++;
        int n = snprintf(buffer_.BeginWrite(),
                         128,
                         "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                         t.tm_year+1900,
                         t.tm_mon+1,
                         t.tm_mday,
                         t.tm_hour,
                         t.tm_min,
                         t.tm_sec,
                         now.tv_usec);
        buffer_.HasWritten(n);
        AppendLogLevelTitle_(level);

        va_start(vaList, format);
        int m = vsnprintf(buffer_.BeginWrite(),
                          buffer_.WritableBytes(),
                          format,
                          vaList);
        va_end(vaList);
        buffer_.HasWritten(m);
        buffer_.Append("\n\0", 2);
        if(is_async_ && deque_ && !deque_->full()){  // 异步 | 队列可用 | 队列未满
            deque_->push_back(buffer_.RetrieveAllToStr());
        }else{  // 同步，在当前线程写入
            fputs(buffer_.Peek(), fp_);
        }
        buffer_.RetrieveAll();  // 清空
    }
}

/// 清除缓冲区，如果异步：唤醒阻塞队列，开始写入
void Log::Flush() {
    if(is_async_){
        deque_->flush();
    }
    fflush(fp_);
}

int Log::GetLevel() {
    lock_guard<mutex> locker(mtx_);
    return level_;
}

void Log::SetLevel(int level) {
    lock_guard<mutex> locker(mtx_);
    level_ = level;
}

Log::Log() {
    fp_ = nullptr;
    deque_ = nullptr;
    write_thread_ = nullptr;
    line_count_ = 0;
    today_ = 0;
    is_async_ = false;
}

/// 日志等级前缀
/// \param level
void Log::AppendLogLevelTitle_(int level) {
    switch (level) {
        case 0:
            buffer_.Append("[debug]: ", 9);
            break;
        case 1:
            buffer_.Append("[info] : ", 9);
            break;
        case 2:
            buffer_.Append("[warn] : ", 9);
            break;
        case 3:
            buffer_.Append("[error]: ", 9);
            break;
        default:
            buffer_.Append("[info] : ", 9);
            break;
    }
}

Log::~Log() {
    while(!deque_->empty()){  // 队列非空，唤醒线程处理
        deque_->flush();
    }
    deque_->close();  // 关闭队列
    write_thread_->join();  // 等待写入线程结束
    if(fp_){
        lock_guard<mutex> locker(mtx_);
        Flush();
        fclose(fp_);
    }
}

/// 写线程执行函数
void Log::AsyncWrite_() {
    string str;
    while(deque_->pop(str)){
        lock_guard<mutex> locker(mtx_);  // 访问fp加锁
        fputs(str.c_str(), fp_);
    }
}
