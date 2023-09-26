//
// Created by 98302 on 2023/9/26.
//

#ifndef WEB_SERVER_TIMER_H
#define WEB_SERVER_TIMER_H

#include <functional>
#include <chrono>

typedef std::function<void()> TimeoutCallback;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

enum TimerType{
    Heap,
    Loop
};

class Timer {
public:
    virtual void Adjust(int id, int new_expire) = 0;
    virtual void Add(int id, int time_out, const TimeoutCallback& callback) = 0;
    virtual void Tick() = 0;
    virtual int GetNextTick() = 0;
};


#endif //WEB_SERVER_TIMER_H
