//
// Created by 98302 on 2023/9/21.
//

#ifndef WEB_SERVER_HTTPCONN_H
#define WEB_SERVER_HTTPCONN_H

#include <arpa/inet.h>  // sockaddr_in
#include "../buffer/Buffer.h"
#include "../log/Log.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

/*
 * 维护一个http连接
 * 提供读取request 写入response
 */
class HttpConn {
public:
    HttpConn();
    ~HttpConn();
    void Init(int sock_fd, const sockaddr_in& addr);
    ssize_t Read(int *save_errno);
    ssize_t Write(int *save_errno);
    void Close();
    int GetFd() const{return fd_;}
    int GetPort() const{return addr_.sin_port;}
    const char* GetIP() const{return inet_ntoa(addr_.sin_addr);}
    sockaddr_in GetAddr() const{return addr_;}
    bool Process();

    int ToWriteBytes(){  // 待写入内容
        return static_cast<int>(iov_[0].iov_len+iov_[1].iov_len);
    }
    bool IsKeepAlive() const{
        return request_.IsKeepAlive();
    }
    static bool is_et;
    static const char* src_dir;
    static std::atomic<int> user_count;  // 原子性访问
private:
    int fd_;  // socket客户端fd
    struct sockaddr_in addr_;
    bool is_closed_;
    int iov_cnt_;
    struct iovec iov_[2];

    Buffer read_buffer_;
    Buffer write_buffer_;

    HttpRequest request_;
    HttpResponse response_;
};


#endif //WEB_SERVER_HTTPCONN_H
