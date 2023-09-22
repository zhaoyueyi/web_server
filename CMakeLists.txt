cmake_minimum_required(VERSION 3.22)
project(web_server)

set(CMAKE_CXX_STANDARD 23)
include_directories(/usr/include/mysql)
add_executable(web_server src/main.cpp
        src/buffer/Buffer.cpp
        src/buffer/Buffer.h
        src/log/Log.cpp
        src/log/Log.h
        src/log/block_queue.h
        src/pool/ThreadPool.h
        src/pool/SqlConnPool.cpp
        src/pool/SqlConnPool.h
        src/http/HttpRequest.cpp
        src/http/HttpRequest.h
        src/http/HttpResponse.cpp
        src/http/HttpResponse.h
        src/http/HttpConn.cpp
        src/http/HttpConn.h
        src/timer/HeapTimer.cpp
        src/timer/HeapTimer.h
        src/server/Epoller.cpp
        src/server/Epoller.h
        src/server/WebServer.cpp
        src/server/WebServer.h
)
target_link_libraries(web_server mysqlclient)
target_link_libraries(web_server pthread)