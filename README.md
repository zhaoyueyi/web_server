# web_server
modern c++ web server(for learning)
### design:
* 单reactor/非阻塞IO/epoll网络模型
* 三段式读写缓冲区
* 小根堆定时器/时间轮定时器，处理超时连接
* 基于阻塞队列单独写线程的异步日志模块
* 自动管理生命周期的RAII的数据库连接池
* 基于条件变量与锁同步的线程池
### reference:
* https://github.com/markparticle/WebServer
* https://github.com/chenshuo/muduo
