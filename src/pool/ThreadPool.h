//
// Created by 98302 on 2023/9/20.
//

#ifndef WEB_SERVER_THREADPOOL_H
#define WEB_SERVER_THREADPOOL_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <cassert>

class ThreadPool {
public:
    ThreadPool() = default;
    ThreadPool(ThreadPool&&) = default;
    explicit ThreadPool(int thread_count = 8): pool_(std::make_shared<Pool>()){
        assert(thread_count > 0);
        for(int i=0; i<thread_count; ++i){  // 创建线程
            std::thread([this](){
                std::unique_lock<std::mutex> locker(pool_->mtx_);  //访问pool，加锁
                while(true){  // 子线程主循环
                    if(!pool_->tasks_.empty()){  // 任务队列非空
                        auto task = std::move(pool_->tasks_.front());  // 拿取任务
                        pool_->tasks_.pop();
                        locker.unlock();  // 解锁
                        task();  // 执行任务，线程安全
                        locker.lock();  // 加锁，准备获取下一个任务
                    }else if(pool_->is_closed_){  // pool关闭，所有子线程执行完任务终止
                        break;
                    }else{  // 任务队列为空，等待加入任务，其他线程等待锁
                        pool_->cv_.wait(locker);
                    }
                }
            }).detach();  // 与主线程分离
        }
    }
    ~ThreadPool(){
        if(pool_){
            std::unique_lock<std::mutex> locker(pool_->mtx_);
            pool_->is_closed_ = true;
        }
        pool_->cv_.notify_all();  // 关闭线程池，唤醒所有wait线程依次终止
    }

    template<typename T>
    void AddTask(T&& task){
        std::unique_lock<std::mutex> locker(pool_->mtx_);
        pool_->tasks_.emplace(std::forward<T>(task));  // 就地构造任务
        pool_->cv_.notify_one();  // 唤醒一个线程执行任务
    }
private:
    struct Pool{
        std::mutex mtx_;
        std::condition_variable cv_;
        bool is_closed_;
        std::queue<std::function<void()>> tasks_;
    };
    std::shared_ptr<Pool> pool_;
};


#endif //WEB_SERVER_THREADPOOL_H
