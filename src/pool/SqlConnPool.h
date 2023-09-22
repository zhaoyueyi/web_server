//
// Created by 98302 on 2023/9/20.
//

#ifndef WEB_SERVER_SQLCONNPOOL_H
#define WEB_SERVER_SQLCONNPOOL_H

#include <queue>
#include <mutex>
#include <mysql/mysql.h>
#include <semaphore>
#include <cassert>

#include "../log/Log.h"

class SqlConnPool {
public:
    static SqlConnPool *Instance();
    MYSQL *GetConn();
    void FreeConn(MYSQL *conn);
    int GetFreeConnCount();
    void Init(const char *host, int port,
              const char *user, const char *pwd,
              const char *db_name, int conn_size);
    void ClosePool();
private:
    SqlConnPool() = default;
    ~SqlConnPool() {ClosePool();}

    int max_conn_;
    std::queue<MYSQL*> conn_queue_;
    std::mutex mtx_;  // 访问connqueue加锁
    sem_t sem_id_;
};
class SqlConnRAII{
public:
    SqlConnRAII(MYSQL** sql, SqlConnPool *conn_pool){
        assert(conn_pool);
        *sql = conn_pool->GetConn();
        sql_ = *sql;
        conn_pool_ = conn_pool;
    }
    ~SqlConnRAII(){
        if(sql_){
            conn_pool_->FreeConn(sql_);
        }
    }
private:
    MYSQL *sql_;
    SqlConnPool *conn_pool_;
};


#endif //WEB_SERVER_SQLCONNPOOL_H
