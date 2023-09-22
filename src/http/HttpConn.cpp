//
// Created by 98302 on 2023/9/21.
//

#include "HttpConn.h"

using namespace std;

const char* HttpConn::src_dir;
atomic<int> HttpConn::user_count;
bool HttpConn::is_et;

HttpConn::HttpConn() {
    fd_ = -1;
    addr_ = {0};
    is_closed_ = true;
}

HttpConn::~HttpConn() {
    Close();
}

void HttpConn::Init(int sock_fd, const sockaddr_in &addr) {
    assert(sock_fd > 0);
    user_count++;
    addr_ = addr;
    fd_ = sock_fd;
    write_buffer_.RetrieveAll();
    read_buffer_.RetrieveAll();
    is_closed_ = false;
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d", fd_, GetIP(), GetPort(), (int)user_count);
}

/// 解析前将socketfd读到读缓冲区
/// \param save_errno
/// \return
ssize_t HttpConn::Read(int *save_errno) {
    ssize_t len = -1;
    do{
        len = read_buffer_.ReadFd(fd_, save_errno);
        if(len <= 0){
            break;
        }
    } while (is_et);  // 边沿触发一次性全部读取
    return len;
}

/// io向量写入socketfd
/// \param save_errno
/// \return 写入字节数，-1：没写入
ssize_t HttpConn::Write(int *save_errno) {
    ssize_t len = -1;
    do {
        len = writev(fd_, iov_, iov_cnt_);
        if(len <= 0){
            *save_errno = errno;
            break;
        }
        if(iov_[0].iov_len + iov_[1].iov_len == 0){  // 无可写内容
            break;
        }else if(static_cast<size_t>(len) > iov_[0].iov_len){
            iov_[1].iov_base = (uint8_t*)iov_[1].iov_base+(len-iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if(iov_[0].iov_len){
                write_buffer_.RetrieveAll();
                iov_[0].iov_len = 0;
            }
        }else{
            iov_[0].iov_base = (uint8_t*)iov_[0].iov_base+len;
            iov_[0].iov_len -= len;
            write_buffer_.Retrieve(len);
        }
    } while (is_et || ToWriteBytes() > 10240);
    return len;
}

void HttpConn::Close() {
    response_.UnmapFile();
    if(!is_closed_){
        is_closed_ = true;
        user_count--;
        close(fd_);
        LOG_INFO("Client[%d](%s:%d) quit, userCount:%d", fd_, GetIP(), GetPort(), (int)user_count);
    }
}

/// 读取读缓冲区，解析request，生成response，放入io向量待写入
/// \return false:无可读内容，true：解析完成
bool HttpConn::Process() {
    request_.Init();
    if(read_buffer_.ReadableBytes() <= 0){  // 无可读内容
        return false;
    }else if(request_.Parse(read_buffer_)){  // request解析成功
        LOG_DEBUG("%s", request_.Path().data());
        response_.Init(src_dir, request_.Path(), request_.IsKeepAlive(), 200);
    }else{  // request解析失败
        response_.Init(src_dir, request_.Path(), false, 400);
    }
    response_.MakeResponse(write_buffer_);
    iov_[0].iov_base = const_cast<char*>(write_buffer_.Peek());  // 响应头
    iov_[0].iov_len = write_buffer_.ReadableBytes();
    iov_cnt_ = 1;
    if(response_.FileLen() > 0 && response_.File()){  // 响应体
        iov_[1].iov_base = response_.File();
        iov_[1].iov_len = response_.FileLen();
        iov_cnt_ = 2;
    }
    LOG_DEBUG("filesize:%d, %d to %d", response_.FileLen(), iov_cnt_, ToWriteBytes());
    return true;
}











