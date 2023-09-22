//
// Created by 98302 on 2023/9/20.
//

#include "Buffer.h"

/// 构造函数：读写下标初始化，容器初始化
/// \param init_size
Buffer::Buffer(int init_size): buffer_(init_size), read_pos_(0), write_pos_(0) {}

/// 可写空间
/// \return
size_t Buffer::WritableBytes() const {
    return buffer_.size() - write_pos_;
}

/// 可读空间
/// \return
size_t Buffer::ReadableBytes() const {
    return write_pos_ - read_pos_;
}

/// 可预留空间
/// \return
size_t Buffer::PrependableBytes() const {
    return read_pos_;
}

/// 读空间起始位置
/// \return
const char *Buffer::Peek() const {
    return &buffer_[read_pos_];
}

/// 确保可写空间，不满足则扩容
/// \param len
void Buffer::EnsureWritable(size_t len) {
    if (len > WritableBytes()){
        MakeSpace_(len);
    }
    assert(len <= WritableBytes());
}

/// 写后，移动写下标
/// \param len
void Buffer::HasWritten(size_t len) {
    write_pos_ += len;
}

/// 读后，移动读下标
/// \param len
void Buffer::Retrieve(size_t len) {
    read_pos_ += len;
}

/// 读到end位置
/// \param end
void Buffer::RetrieveUntil(const char *end) {
    assert(Peek() <= end);
    Retrieve(end - Peek());
}

/// 取出所有数据，buffer归零，读写下标归零
/// \param len
void Buffer::RetrieveAll() {
    bzero(&buffer_[0], buffer_.size());
    read_pos_ = write_pos_ = 0;
}

std::string Buffer::RetrieveAllToStr() {
    std::string str(Peek(), ReadableBytes());
    RetrieveAll();
    return str;
}

/// 写指针位置
/// \return
const char *Buffer::BeginWriteConst() const {
    return &buffer_[write_pos_];
}
char *Buffer::BeginWrite() {
    return &buffer_[write_pos_];
}

/// str写入缓冲区
/// \param str
void Buffer::Append(const std::string &str) {
    Append(str.c_str(), str.size());
}
// core
void Buffer::Append(const char *str, size_t len) {
    assert(str);  // str not null
    EnsureWritable(len);  // 确保写入空间
    std::copy(str, str+len, BeginWrite());  // 写入，copy：容器元素复制
    HasWritten(len);  //移动写指针
}

void Buffer::Append(const void *data, size_t len) {
    Append(static_cast<const char*>(data), len);
}

void Buffer::Append(const Buffer &buffer) {
    Append(buffer.Peek(), buffer.ReadableBytes());
}


/// 客户端IO读写接口，读到缓冲区
/// \param fd
/// \param err_no 返回异常码
/// \return  读取内容大小
ssize_t Buffer::ReadFd(int fd, int *err_no) {
    char buff[65535];  // 临时栈空间
    struct iovec iov[2];  // io向量，0,1...顺序使用，readv和writev
    size_t writable = WritableBytes();  // 可写空间
    iov[0].iov_base = BeginWrite();
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);
    ssize_t len = readv(fd, iov, 2);
    if(len < 0){  // 读取异常
        *err_no = errno;
    }else if(static_cast<size_t>(len) <= writable){  // 可写空间满足读取大小
        write_pos_ += len;  // 移动写下标
    }else{  // 可写空间已满
        write_pos_ = buffer_.size();  // 写下标移动到末尾
        Append(buff, static_cast<size_t>(len - writable));
    }
    return len;
}

ssize_t Buffer::WriteFd(int fd, int *err_no) {
    ssize_t len = write(fd, Peek(), ReadableBytes());
    if(len < 0){
        *err_no = errno;
        return len;
    }
    Retrieve(len);
    return len;
}

char* Buffer::BeginPtr_() {
    return &buffer_[0];
}

const char *Buffer::BeginPtr_() const {
    return &buffer_[0];
}

/// 扩容
/// \param len
void Buffer::MakeSpace_(size_t len) {
    if(ReadableBytes() + PrependableBytes() < len){  // 预留空间不足
        buffer_.resize(write_pos_+len+1);
    }else{  // 使用预留空间
        size_t readable = ReadableBytes();
        std::copy(BeginPtr_()+read_pos_, BeginPtr_()+write_pos_, BeginPtr_());  //前移
        read_pos_ = 0;
        write_pos_ = readable;
        assert(readable == ReadableBytes());
    }
}
