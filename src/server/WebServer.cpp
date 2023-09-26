//
// Created by 98302 on 2023/9/21.
//

#include "WebServer.h"

WebServer::WebServer(int port, int trigger_mode, TimerType timer_type, int timeout_ms, bool opt_linger, int sql_port,
                     const char *sql_username, const char *sql_password, const char *db_name, int conn_pool_num,
                     int thread_num, bool open_log, int log_level, int log_queue_size):
                     port_(port), open_linger_(opt_linger), timeout_ms_(timeout_ms), is_closed_(false),
                     thread_pool_(new ThreadPool(thread_num)),
                     epoller_(new Epoller()){
    src_dir_ = getcwd(nullptr, 256);  // 当前工作目录
    assert(src_dir_);
    strcat(src_dir_, "/resources/");
    // 初始化日志
    if(open_log) {
        Log::Instance()->Init(log_level,
                              "./log",
                              ".log",
                              log_queue_size);
    }
    // 初始化HttpConn静态变量
    HttpConn::user_count = 0;
    HttpConn::src_dir = src_dir_;
    // 初始化SqlPool
    SqlConnPool::Instance()->Init("localhost",
                                  sql_port,
                                  sql_username,
                                  sql_password,
                                  db_name,
                                  conn_pool_num);
    // 初始化Timer
    switch (timer_type) {
        case TimerType::Heap:
            timer_ = make_unique<HeapTimer>();
            break;
        case TimerType::Loop:
        default:
            timer_ = make_unique<LoopTimer>();
            break;
    }
    // 初始化事件 socket
    InitEventMode_(trigger_mode);
    if(!InitSocket_()){
        is_closed_ = true;
    }

    if(is_closed_){
        LOG_ERROR("================Server init error!================");
    }else{
        LOG_INFO("================Server init================");
        LOG_INFO("Port:%d, OpenLinger:%s", port_, open_linger_ ?"true":"false");
        LOG_INFO("Listen mode:%s, OpenConn mode:%s",
                 (listen_event_ & EPOLLET?"ET":"LT"),
                 (conn_event_ & EPOLLET?"ET":"LT"));
        LOG_INFO("Log level:%d", log_level);
        LOG_INFO("Src Dir:%s", HttpConn::src_dir);
        LOG_INFO("Sql Conn Pool num:%d, thread-pool num:%d", conn_pool_num, thread_num);
    }
}

WebServer::~WebServer() {
    close(listen_fd_);
    is_closed_ = true;
    free(src_dir_);
    SqlConnPool::Instance()->ClosePool();
}

void WebServer::Start() {
    int timeout = -1;  // 无事件阻塞
    if(!is_closed_){
        LOG_INFO("================Server start================");
    }
    while(!is_closed_){
        if(timeout_ms_ > 0){
            timeout = timer_->GetNextTick();  // 最近剩余过期时间
        }
        int event_cnt = epoller_->Wait(timeout);  // 当前就绪队列中事件数
        for(int i=0; i<event_cnt; ++i){
            // 处理事件
            int fd = epoller_->GetEventFd(i);
            uint32_t events = epoller_->GetEvents(i);
            if(fd == listen_fd_){  // 收到请求连接socket事件
                DealListen_();
            }else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)){
                assert(users_.count(fd) > 0);
                CloseConn_(&users_[fd]);
            }else if(events & EPOLLIN){  // 收到读事件，客户端发来数据
                assert(users_.count(fd) > 0);
                DealRead_(&users_[fd]);
            }else if(events & EPOLLOUT){  // 收到写事件，服务端准备好数据
                assert(users_.count(fd) > 0);
                DealWrite_(&users_[fd]);
            }else{
                LOG_ERROR("Unexpected event");
            }
        }

    }
}

bool WebServer::InitSocket_() {
    int ret;
    struct sockaddr_in addr{};
    if(port_ > 65535 || port_ < 1024){
        LOG_ERROR("Port:%d error!", port_);
        return false;
    }
    addr.sin_family = AF_INET;  // ipv4
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port_);

    {
        struct linger opt_linger = {0};
        if(open_linger_){
            opt_linger.l_onoff = 1;
            opt_linger.l_linger = 1;
        }
//        listen_fd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if(listen_fd_ < 0){
            LOG_ERROR("Create socket error!");
            return false;
        }
        ret = setsockopt(listen_fd_,
                         SOL_SOCKET,
                         SO_LINGER,
                         &opt_linger,
                         sizeof(opt_linger));
        if(ret < 0){
            close(listen_fd_);
            LOG_ERROR("Init linger error!");
            return false;
        }
    }
    int optval = 1;
    ret = setsockopt(listen_fd_,
                     SOL_SOCKET,
                     SO_REUSEADDR,
                     (const void*)&optval,
                     sizeof(int));
    if(ret < 0){
        LOG_ERROR("Set port reuse error!");
        return false;
    }
    ret = bind(listen_fd_, (struct sockaddr*)&addr, sizeof(addr));
    if(ret < 0){
        LOG_ERROR("Bind port:%d error!", port_);
        return false;
    }
    ret = listen(listen_fd_, 6);
    if(ret < 0){
        LOG_ERROR("Listen port:%d error!", port_);
        return false;
    }
    ret = epoller_->AddFd(listen_fd_, listen_event_ | EPOLLIN);  // 将接入请求事件加入epoll
    if(ret == 0){
        LOG_ERROR("Add listen error!");
        close(listen_fd_);
        return false;
    }
    SetFdNonblock_(listen_fd_);
    LOG_INFO("Server port:%d", port_);
    return true;
}

/// 初始化fd触发方式标志位（ET LT）
/// \param trigger_mode
void WebServer::InitEventMode_(int trigger_mode) {
    listen_event_ = EPOLLRDHUP;
    conn_event_ = EPOLLONESHOT | EPOLLRDHUP;
    switch (trigger_mode) {
        case 0:
            break;
        case 1:
            conn_event_ |= EPOLLET;
            break;
        case 2:
            listen_event_ |= EPOLLET;
            break;
        case 3:
        default:
            listen_event_ |= EPOLLET;
            conn_event_ |= EPOLLET;
            break;
    }
    HttpConn::is_et = (conn_event_ & EPOLLET);
}

/// 客户端加入
/// \param fd
/// \param addr
void WebServer::AddClient_(int fd, sockaddr_in addr) {
    assert(fd > 0);
    users_[fd].Init(fd, addr);
    if (timeout_ms_ > 0) {
        timer_->Add(fd,
                    timeout_ms_,
                    [this, conn = &users_[fd]] { CloseConn_(conn); });  // 绑定超时断连回调
    }
    epoller_->AddFd(fd, EPOLLIN | conn_event_);  // 监听客户端fd的写入事件
    SetFdNonblock_(fd);//todo
    LOG_INFO("Client[%d] in!", users_[fd].GetFd());
}

/// 处理客户端连接请求
void WebServer::DealListen_() {
    struct sockaddr_in addr{};
    socklen_t len = sizeof(addr);
    do{
        int fd = accept(listen_fd_, (struct sockaddr*)&addr, &len);//todo
//        int fd = accept4(listen_fd_, (struct sockaddr*)&addr, &len, SOCK_NONBLOCK);
        if(fd <= 0){
            return;
        }else if(HttpConn::user_count >= MAX_FD){
            SendError_(fd, "Server busy!");
            LOG_WARN("Clients is too many!");
            return;
        }
        AddClient_(fd, addr);
    } while (listen_event_ & EPOLLET);
}

void WebServer::DealWrite_(HttpConn *client) {
    assert(client);
    ExtentTime_(client);
    thread_pool_->AddTask([this, client]{ OnWrite_(client);});
}

void WebServer::DealRead_(HttpConn *client) {
    assert(client);
    ExtentTime_(client);
    thread_pool_->AddTask([this, client]{ OnRead_(client);});
}

void WebServer::SendError_(int fd, const char *info) {
    assert(fd > 0);
    ssize_t ret = send(fd, info, strlen(info), 0);
    if(ret < 0){
        LOG_WARN("Send error info to client:%s failed!", fd);
    }
    close(fd);
}

/// 延长客户端连接，防止超时断连
/// \param client
void WebServer::ExtentTime_(HttpConn *client) {
    assert(client);
    if(timeout_ms_ > 0){
        timer_->Adjust(client->GetFd(), timeout_ms_);
    }
}

void WebServer::CloseConn_(HttpConn *client) {
    assert(client);
    LOG_INFO("Client[%d] disconnect!", client->GetFd());
    epoller_->DelFd(client->GetFd());
    client->Close();
}

/// 子线程读任务
/// \param client
void WebServer::OnRead_(HttpConn *client) {
    assert(client);
    ssize_t ret = -1;
    int read_errno = 0;
    ret = client->Read(&read_errno);
    if(ret <= 0 && read_errno != EAGAIN){  // 读失败，并且没有后续，断连
        CloseConn_(client);
        return;
    }
    OnProcess(client);  // 读成功，继续解析
}

/// 子线程写任务
/// \param client
void WebServer::OnWrite_(HttpConn *client) {
    assert(client);
    ssize_t ret = -1;
    int write_errno = 0;
    ret = client->Write(&write_errno);
    if(client->ToWriteBytes() == 0){  // 没有更多内容需要写入
        if(client->IsKeepAlive()){  // 维持长连接
            epoller_->ModFd(client->GetFd(), conn_event_ | EPOLLIN);  // 监听读事件
            return;
        }
    }else if(ret < 0){  // 此次未写入
        if(write_errno == EAGAIN){  // 继续传输, 继续监听写事件
            epoller_->ModFd(client->GetFd(), conn_event_ | EPOLLOUT);
            return;  // 中断退出，未断连
        }
    }
    CloseConn_(client);  // 未keepalive并且无须继续传输，断连
}

/// 读入缓冲区完毕，解析内容，并生成响应到缓冲区，生成完毕准备写事件就绪
/// \param client
void WebServer::OnProcess(HttpConn *client) {
    if(client->Process()){
        epoller_->ModFd(client->GetFd(), conn_event_ | EPOLLOUT);  // 读成功，继续监听读事件
    }else{  // 无可读内容
        epoller_->ModFd(client->GetFd(), conn_event_ | EPOLLIN);  // 读取完毕，监听写事件
    }
}

/// fd设置非阻塞
/// \param fd
/// \return
//int WebServer::SetFdNonblock_(int fd) {
//    assert(fd > 0);
//    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
//}
int WebServer::SetFdNonblock_(int fd) {
    int flag = fcntl(fd, F_GETFL, 0);
    if (flag == -1) return -1;

    flag |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flag) == -1) return -1;
    return 0;
}