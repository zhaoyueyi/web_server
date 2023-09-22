//
// Created by 98302 on 2023/9/20.
//

#include "SqlConnPool.h"

/// 获取单例，懒汉模式
/// \return
SqlConnPool *SqlConnPool::Instance() {
    static SqlConnPool pool;
    return &pool;
}

/// 连接池取用连接
/// \return
MYSQL *SqlConnPool::GetConn() {
    MYSQL *conn = nullptr;
    if(conn_queue_.empty()){  // 连接池已耗尽
        LOG_WARN("SqlConnPool busy!");
        return nullptr;
    }
    sem_wait(&sem_id_);  // -1
    lock_guard<mutex> locker(mtx_);
    conn = conn_queue_.front();
    conn_queue_.pop();
    return conn;
}

/// 释放连接回连接池
/// \param conn
void SqlConnPool::FreeConn(MYSQL *conn) {
    assert(conn);
    lock_guard<mutex> locker(mtx_);
    conn_queue_.push(conn);
    sem_post(&sem_id_);  // +1
}

int SqlConnPool::GetFreeConnCount() {
    lock_guard<mutex> locker(mtx_);
    return static_cast<int>(conn_queue_.size());
}

/// 初始化连接池
/// \param host
/// \param port
/// \param user
/// \param pwd
/// \param db_name
/// \param conn_size
void
SqlConnPool::Init(const char *host, int port, const char *user, const char *pwd, const char *db_name, int conn_size) {
    assert(conn_size > 0);
    for(int i=0; i<conn_size; ++i){  // 创建连接
        MYSQL *conn = nullptr;
        conn = mysql_init(conn);
        if(!conn){
            LOG_ERROR("MySQL Init error!");
            assert(conn);
        }
        conn = mysql_real_connect(conn, host, user, pwd, db_name, port, nullptr, 0);
        if(!conn){
            LOG_ERROR("MySQL connect error!");
        }
        conn_queue_.emplace(conn);
    }
    max_conn_ = conn_size;
    sem_init(&sem_id_, 0, max_conn_);  // 信号量初始化为最大连接数
}

/// 关闭连接池，逐个关闭连接
void SqlConnPool::ClosePool() {
    lock_guard<mutex> locker(mtx_);
    while (!conn_queue_.empty()){
        auto conn = conn_queue_.front();
        conn_queue_.pop();
        mysql_close(conn);
    }
    mysql_library_end();
}
