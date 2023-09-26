//
// Created by 98302 on 2023/9/26.
//

#include "LoopTimer.h"
using namespace std;
void LoopTimer::Adjust(int id, int new_expire) {
    assert(!list_.empty() && map_.count(id));
    if(map_[id].expired()){
        map_.erase(id);
    }else{
        auto node = map_[id].lock();
        node->expire = Clock::now() + MS(new_expire);
        list_.emplace_back(node);
    }
}

void LoopTimer::Add(int id, int time_out, const TimeoutCallback &callback) {
    assert(id >= 0);
    if(map_.find(id) == map_.end()){  // 新建
        list_.emplace_back(std::make_shared<LoopTimerNode>(id, Clock::now()+MS(time_out), callback));
        map_[id] = std::weak_ptr(list_.back());
    }else{
        if(map_[id].expired()){
            list_.emplace_back(std::make_shared<LoopTimerNode>(id, Clock::now()+MS(time_out), callback));
            map_[id] = std::weak_ptr(list_.back());
        }else{
            Adjust(id, time_out);
        }
    }
    if(list_.size() == 1) cur_ = list_.begin();
}

void LoopTimer::Tick() {
    while(!list_.empty()){
        auto node_iter = cur_;
        if(++cur_ == list_.end()) cur_ = list_.begin();
        if(chrono::duration_cast<MS>((*node_iter)->expire-Clock::now()).count() > 0){
            break;
        }
        list_.erase(node_iter);
    }
}

void LoopTimer::Clear() {
    list_.clear();
    map_.clear();
}

int LoopTimer::GetNextTick() {
    Tick();
    return -1;
}
