#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "../../http/http_conn.h"

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <functional>
#include <cassert>

class ThreadPool
{
public:
    ThreadPool(size_t thread_count = 8);
    ThreadPool() = default;
    ThreadPool(ThreadPool &&) = default;
    ~ThreadPool();

    void AddTask(std::function<void()> &&task);

private:
    struct Pool
    {
        std::mutex mtx;
        std::condition_variable cond;
        bool is_closed;
        std::queue<std::function<void()>> tasks;
    };
    std::shared_ptr<Pool> pool_;
};

#endif // THREADPOOL_H