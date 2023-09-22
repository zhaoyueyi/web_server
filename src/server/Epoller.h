//
// Created by 98302 on 2023/9/21.
//

#ifndef WEB_SERVER_EPOLLER_H
#define WEB_SERVER_EPOLLER_H

#include <sys/epoll.h>
#include <vector>
#include <cassert>
#include <unistd.h>

/*
 * epoll封装类
 * IO多路复用：多socket复用一个线程
 * 与客户端连接需维护一个fd
 * select：轮询fd
 * epoll：红黑树存储fd，有事件时放入就绪链表，唤醒wait
 * epoll_create => epoll实例fd
 * epoll_ctl (epoll_fd, {EPOLL_CTL_ADD添加事件 | EPOLL_CTL_MOD修改事件 | EPOLL_CTL_DEL删除事件}, 需要检测的fd, 需要检测的事件)
 * epoll_wait (epoll_fd, event导出事件, 事件数量, 阻塞时间{0不阻塞 | -1阻塞至fd变化 | +阻塞毫秒}) => 变化的fd个数
 * epoll events:
 *      EPOLLIN      fd可读
 *      EPOLLOUT     fd可写
 *      EPOLLPRI     fd紧急可读
 *      EPOLLERR     fd错误
 *      EPOLLHUP     fd被挂断
 *      EPOLLET      边沿触发
 *      EPOLLONESHOT 监听一次
 * LT水平触发：接收缓冲区不为空，读事件一直触发
 *           发送缓冲区不为空，写事件一直触发
 * ET边沿触发：
 */
class Epoller {
public:
    explicit Epoller(int max_event = 1024);
    ~Epoller();

    bool AddFd(int fd, uint32_t events) const;
    bool ModFd(int fd, uint32_t events) const;
    bool DelFd(int fd) const;
    int Wait(int time_out_ms = -1);
    int GetEventFd(size_t i) const;
    uint32_t GetEvents(size_t i) const;
private:
    int epoll_fd_;
    std::vector<struct epoll_event> events_;
};


#endif //WEB_SERVER_EPOLLER_H
