//
// Created by 98302 on 2023/9/21.
//

#ifndef WEB_SERVER_WEBSERVER_H
#define WEB_SERVER_WEBSERVER_H

#include <netinet/in.h>
#include "../http/HttpConn.h"
#include "../timer/Timer.h"
#include "../timer/HeapTimer.h"
#include "../timer/LoopTimer.h"
#include "../pool/ThreadPool.h"
#include "Epoller.h"

/*
 * web server主类
 * 1. 监听内核IO事件
 *      读=>放入读任务队列
 *      写=>放入写任务队列
 * 2. 处理超时连接
 * 初始化：
 *      创建线程池
 *      初始化socket
 *      初始化epoller
 *      初始化数据库连接池
 *      初始化日志系统
 * 启动：
 *      Reactor 读写事件交给工作线程处理
 *      客户端/浏览器向服务器发送request
 *      epoller收到EPOLLIN
 *      解析，将request放入读缓冲区
 *      生成response，放入写缓冲区
 *      事件改为EPOLLOUT，将写缓冲区内容写入fd
 *      置回EPOLLIN监听读事件
 */
class WebServer {
public:
    WebServer(int port,
              int trigger_mode,
              TimerType timer_type,
              int timeout_ms,
              bool opt_linger,
              int sql_port,
              const char* sql_username,
              const char* sql_password,
              const char* db_name,
              int conn_pool_num,
              int thread_num,
              bool open_log,
              int log_level,
              int log_queue_size);
    ~WebServer();
    void Start();
private:
    bool InitSocket_();
    void InitEventMode_(int trigger_mode);
    void AddClient_(int fd, sockaddr_in addr);

    void DealListen_();
    void DealWrite_(HttpConn *client);
    void DealRead_(HttpConn *client);

    static void SendError_(int fd, const char* info);
    void ExtentTime_(HttpConn *client);
    void CloseConn_(HttpConn *client);

    void OnRead_(HttpConn *client);
    void OnWrite_(HttpConn *client);
    void OnProcess(HttpConn *client);

    static const int MAX_FD = 65536;
    static int SetFdNonblock_(int fd);

    int port_;
    bool open_linger_;
    int timeout_ms_;
    bool is_closed_;
    int listen_fd_;
    char* src_dir_;  // 申请时malloc，手动free

    uint32_t listen_event_;
    uint32_t conn_event_;  // 是否ET标记位

    std::unique_ptr<Timer> timer_;
    std::unique_ptr<ThreadPool> thread_pool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int, HttpConn> users_;
};


#endif //WEB_SERVER_WEBSERVER_H
