//
// Created by 98302 on 2023/9/21.
//

#ifndef WEB_SERVER_HEAPTIMER_H
#define WEB_SERVER_HEAPTIMER_H

#include <functional>
#include <chrono>
#include <cassert>
#include <algorithm>

typedef std::function<void()> TimeoutCallback;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

struct TimerNode{
    int id;
    TimeStamp expire;
    TimeoutCallback callback;
    bool operator < (const TimerNode& t) const{
        return expire < t.expire;
    }
    bool operator > (const TimerNode& t) const{
        return expire > t.expire;
    }
};

/*
 * 小根堆定时器
 */
class HeapTimer {
public:
    HeapTimer(){heap_.reserve(64);}
    ~HeapTimer(){Clear();}

    void Adjust(int id, int new_expire);
    void Add(int id, int time_out, const TimeoutCallback& callback);
    void DoWork(int id);
    void Tick();
    void Clear();
    void Pop();
    int GetNextTick();
private:
    void Del_(size_t i);
    void ShiftUp_(size_t i);
    bool ShiftDown_(size_t i, size_t n);
    void SwapNode_(size_t i, size_t j);

    std::vector<TimerNode> heap_;
    std::unordered_map<int, size_t> ref_;
};


#endif //WEB_SERVER_HEAPTIMER_H
