//
// Created by 98302 on 2023/9/20.
//

#ifndef WEB_SERVER_BUFFER_H
#define WEB_SERVER_BUFFER_H

#include <cstring>
#include <string>
#include <vector>
#include <atomic>

#include <cassert>
#include <sys/uio.h>

/**
 * 缓冲区，暂存报文、日志
 * io向量：避免反复read，减少系统调用
 */
class Buffer {
public:
    explicit Buffer(int init_size = 1024);
    ~Buffer() = default;

    size_t WritableBytes() const;
    size_t ReadableBytes() const;
    size_t PrependableBytes() const;

    const char* Peek() const;
    void EnsureWritable(size_t len);
    void HasWritten(size_t len);

    void Retrieve(size_t len);
    void RetrieveUntil(const char* end);
    void RetrieveAll();
    std::string RetrieveAllToStr();

    const char* BeginWriteConst() const;
    char* BeginWrite();

    void Append(const std::string& str);
    void Append(const char* str, size_t len);
    void Append(const void* data, size_t len);
    void Append(const Buffer& buffer);

    ssize_t ReadFd(int fd, int* err_no);
    ssize_t WriteFd(int fd, int* err_no);
private:
    char* BeginPtr_();
    const char* BeginPtr_() const;
    void MakeSpace_(size_t len);

    /*
     *    prependable    readable    writable
     * |______________|___________|___________|
     * 0          read_pos    write_pos     size()
     */
    std::vector<char> buffer_;  // 缓冲区主体，读写共用
    std::atomic<size_t> read_pos_;  // 当前读指针，使用atomic保证多线程同步
    std::atomic<size_t> write_pos_;  // 当前写指针
};


#endif //WEB_SERVER_BUFFER_H
