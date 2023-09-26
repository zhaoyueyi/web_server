//
// Created by 98302 on 2023/9/26.
//

#ifndef WEB_SERVER_LOOPTIMER_H
#define WEB_SERVER_LOOPTIMER_H
#include <functional>
#include <chrono>
#include <cassert>
#include <algorithm>
#include <list>
#include <unordered_map>
#include <memory>
#include "Timer.h"

typedef std::function<void()> TimeoutCallback;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

struct LoopTimerNode{
    int id;
    TimeStamp expire;
    TimeoutCallback callback;
    ~LoopTimerNode(){
        callback();
    }
};

/*
 * 时间轮定时器
 */
class LoopTimer: public Timer {
public:
    LoopTimer() = default;
    ~LoopTimer(){Clear();}

    void Adjust(int id, int new_expire) override;
    void Add(int id, int time_out, const TimeoutCallback& callback) override;
    void Tick() override;
    void Clear();
    int GetNextTick() override;
private:
    std::list<std::shared_ptr<LoopTimerNode>> list_;
    std::list<std::shared_ptr<LoopTimerNode>>::iterator cur_;
    std::unordered_map<int, std::weak_ptr<LoopTimerNode>> map_;
};

#endif //WEB_SERVER_LOOPTIMER_H
