//
// Created by 98302 on 2023/9/21.
//

#include "HeapTimer.h"
#define check_range(n) assert((n) >= 0 && (n) < heap_.size())

using namespace std;

void HeapTimer::Adjust(int id, int new_expire) {
    assert(!heap_.empty() && ref_.count(id));
    heap_[ref_[id]].expire = Clock::now() + MS(new_expire);
    ShiftDown_(ref_[id], heap_.size());
}

void HeapTimer::Add(int id, int time_out, const TimeoutCallback &callback) {
    assert(id >= 0);
    if(ref_.count(id)){
        int tmp = static_cast<int>(ref_[id]);
        heap_[tmp].expire = Clock::now() + MS(time_out);
        heap_[tmp].callback = callback;
        if(!ShiftDown_(tmp, heap_.size())){
            ShiftUp_(tmp);
        }
    }else{
        size_t n = heap_.size();
        ref_[id] = n;
        heap_.emplace_back(id, Clock::now()+MS(time_out), callback);
        ShiftUp_(n);
    }
}

void HeapTimer::DoWork(int id) {
    if(heap_.empty() || ref_.count(id) == 0){
        return;
    }
    size_t i = ref_[id];
    auto node = heap_[i];
    node.callback();
    Del_(i);
}

void HeapTimer::Tick() {
    while(!heap_.empty()){
        TimerNode node = heap_.front();
        if(chrono::duration_cast<MS>(node.expire-Clock::now()).count() > 0){
            break;
        }
        node.callback();
        Pop();
    }
}

void HeapTimer::Clear() {
    ref_.clear();
    heap_.clear();
}

void HeapTimer::Pop() {
    assert(!heap_.empty());
    Del_(0);
}

int HeapTimer::GetNextTick() {
    Tick();
    size_t res = -1;
    if(!heap_.empty()){
        res = chrono::duration_cast<MS>(heap_.front().expire-Clock::now()).count();  // res never < 0??????
    }
    return static_cast<int>(res);
}

void HeapTimer::Del_(size_t i) {
    check_range(i);
    size_t tmp = i;
    size_t n = heap_.size()-1;
    assert(tmp <= n);
    if(i < heap_.size()-1){ // ?????
        SwapNode_(tmp, heap_.size() - 1);
        if(!ShiftDown_(tmp, n)){
            ShiftUp_(tmp);
        }
    }
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}

/// todo
/// \param i
void HeapTimer::ShiftUp_(size_t i) {
    check_range(i);
    size_t parent = (i-1) / 2;
    while (parent >= 0){
        if(heap_[parent] > heap_[i]){
            SwapNode_(i, parent);
            i = parent;
            parent = (i-1) / 2;
        }else{
            break;
        }
    }
}

/// 下移节点，false为未下移，true为下移
/// \param i
/// \param n
/// \return
bool HeapTimer::ShiftDown_(size_t i, size_t n) {
    check_range(i);
    assert(n >= 0 && n <= heap_.size());
    auto index = i;
    auto child = 2*index+1;
    while(child < n){
        if(child + 1 < n && heap_[child] > heap_[child+1]){
            child++;
        }
        if(heap_[child] < heap_[index]){
            SwapNode_(child, index);
            index = child;
            child = 2*child+1;
        }
        break;
    }
    return index > i;
}

void HeapTimer::SwapNode_(size_t i, size_t j) {
    check_range(i);
    check_range(j);
    swap(heap_[i], heap_[j]);
    ref_[heap_[i].id] = i;
    ref_[heap_[j].id] = j;
}
