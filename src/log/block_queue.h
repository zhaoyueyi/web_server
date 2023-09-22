//
// Created by 98302 on 2023/9/20.
//

#ifndef WEB_SERVER_BLOCK_QUEUE_H
#define WEB_SERVER_BLOCK_QUEUE_H

#include <cstring>
#include <deque>
#include <mutex>
#include <condition_variable>

#include <cassert>

using namespace std;

template<typename T>
class BlockQueue{
public:
    explicit BlockQueue(size_t max_size = 1000): capacity_(max_size) {  // capacity初始化后不可变，当capacity为0，同步日志，无须队列
        assert(max_size > 0);
        is_closed_ = false;
    }// 防止隐式转换

    ~BlockQueue() {
        close();
    }

    bool empty() {
        lock_guard<mutex> locker(mtx_);  // lock_guard加锁，离开作用域自动释放锁，不可移动和复制
        return deq_.empty();
    }

    bool full() {
        lock_guard<mutex> locker(mtx_);
        return deq_.size() >= capacity_;
    }

    /// 生产者
    /// \param item
    void push_back(const T& item) {
        unique_lock<mutex> locker(mtx_);  // unique_lock可以移动，不可复制，可以手动lock&unlock
        while (deq_.size() >= capacity_){  // 队列已满，需要消费
            cv_producer_.wait(locker);  // 暂停push，等待消费者消费后唤醒生产cv，先unlock锁，当前线程休眠，被唤醒后重新lock锁
        }
        deq_.push_back(item);  //生产
        cv_consumer_.notify_one();  // 随机唤醒wait的消费者
    }

    void push_front(const T& item) {
        unique_lock<mutex> locker(mtx_);
        while (deq_.size() >= capacity_){
            cv_producer_.wait(locker);
        }
        deq_.push_front(item);
        cv_consumer_.notify_one();
    }

    /// 消费者
    /// \param item 消费内容返回
    /// \return 成功消费
    bool pop(T& item) {
        unique_lock<mutex> locker(mtx_);  // 加锁
        while (deq_.empty()){
            cv_consumer_.wait(locker);  // 队列为空时 unlock 消费者休眠，等待生产者唤醒，唤醒后 lock
        }
        item = deq_.front();
        deq_.pop_front();  // 消费
        cv_producer_.notify_one();  // 唤醒一个wait的生产者
        return true;
    }

    // 设置超时时间的消费者
    bool pop(T& item, int timeout) {
        unique_lock<mutex> locker(mtx_);
        while (deq_.empty()){
            if(cv_consumer_.wait_for(locker, std::chrono::seconds(timeout)) == std::cv_status::timeout){  // wait_for设置超时的wait，返回枚举是否timeout状态
                return false;
            }
            if(is_closed_){  // 队列是否已关闭
                return false;
            }
        }
        item = deq_.front();
        deq_.pop_front();
        cv_producer_.notify_one();
        return true;
    }

    void clear() {
        lock_guard<mutex> locker (mtx_);
        deq_.clear();
    }

    T front() {
        lock_guard<mutex> locker(mtx_);
        return deq_.front();
    }

    T back() {
        lock_guard<mutex> locker(mtx_);
        return deq_.back();
    }

    size_t capacity() {
        lock_guard<mutex> locker(mtx_);  // ********
        return capacity_;
    }

    size_t size() {
        lock_guard<mutex> locker(mtx_);
        return deq_.size();
    }

    /// 唤醒消费者
    void flush() {
        cv_consumer_.notify_one();
    }

    void close() {
        clear();
        is_closed_ = true;
        cv_consumer_.notify_all();
        cv_producer_.notify_all();
    }

private:
    deque<T> deq_;
    mutex mtx_;  // 访问deq加锁
    bool is_closed_;
    size_t capacity_;
    condition_variable cv_consumer_; // 用于生产消费的条件变量
    condition_variable cv_producer_;
};


#endif //WEB_SERVER_BLOCK_QUEUE_H
