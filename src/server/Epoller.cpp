//
// Created by 98302 on 2023/9/21.
//

#include "Epoller.h"

Epoller::Epoller(int max_event): epoll_fd_(epoll_create(512)), events_(max_event) {
    assert(epoll_fd_ >= 0 && !events_.empty());
}

Epoller::~Epoller() {
    close(epoll_fd_);
}

bool Epoller::AddFd(int fd, uint32_t events) const {
    if(fd < 0){
        return false;
    }
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);
}

bool Epoller::ModFd(int fd, uint32_t events) const {
    if(fd < 0){
        return false;
    }
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
}

bool Epoller::DelFd(int fd) const {
    if(fd < 0){
        return false;
    }
    return 0 == epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, 0);
}

/// 等待事件触发
/// \param time_out_ms
/// \return
int Epoller::Wait(int time_out_ms) {
    return epoll_wait(epoll_fd_,
                      &events_[0],
                      static_cast<int>(events_.size()),
                      time_out_ms);  // 从fd中读取触发事件放入events
}

int Epoller::GetEventFd(size_t i) const {
    assert(i >= 0 && i < events_.size());
    return events_[i].data.fd;
}

uint32_t Epoller::GetEvents(size_t i) const {
    assert(i >= 0 && i < events_.size());
    return events_[i].events;
}
