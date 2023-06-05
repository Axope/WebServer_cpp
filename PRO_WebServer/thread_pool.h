#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "locker.h"
#include <deque>
#include <iostream>

// 线程池类
template<typename T>
class thread_pool
{
private:
    int m_thread_num;               // 线程数量
    pthread_t* m_threads;           // 线程池数组,大小等于m_thread_num

    int m_max_requests;             // 请求队列的最大请求数量
    std::deque<T*> m_workqueue;     // 请求队列,大小等于m_max_requests
    locker m_queue_locker;          // 保护请求队列的互斥锁
    sem m_queue_stat;               // 是否有任务需要处理

    bool m_stop;                    // 停止标记

private:
    static void* work(void* arg);   // 工作函数
    void run();                     // 这个才是真正处理流程的函数

public:
    thread_pool(int thread_number = 8, int max_requests = 100000);
    ~thread_pool();
    bool append(T* request);        // 向请求队列加入一个任务
};

template<typename T>
bool thread_pool<T>::append(T* request)
{
    m_queue_locker.lock();
    if (m_workqueue.size() >= m_max_requests)
    {
        m_queue_locker.unlock();
        return false;
    }
    m_workqueue.push_back(request);
    m_queue_locker.unlock();
    m_queue_stat.post();
    return true;
}

template<typename T>
thread_pool<T>::thread_pool(int thread_number, int max_requests) :
    m_thread_num(thread_number), m_threads(nullptr),
    m_max_requests(max_requests), m_stop(false)
{
    m_threads = new pthread_t[m_thread_num];
    if (m_threads == nullptr)
    {
        throw std::exception();
    }

    // 创建num个线程并将他们设置detach
    for (int i = 0; i < thread_number; i++)
    {
        std::cout << "Create the " << i << "th thread..." << std::endl;
        if (pthread_create(m_threads + i, NULL, work, this) != 0)
        {
            delete[] m_threads;
            throw std::exception();
        }

        if (pthread_detach(m_threads[i]))
        {
            delete[] m_threads;
            throw std::exception();
        }
    }

}

template<typename T>
thread_pool<T>::~thread_pool()
{
    delete[] m_threads;
    m_stop = true;
}

template<typename T>
void* thread_pool<T>::work(void* arg)
{
    thread_pool* pool = (thread_pool*)arg;
    pool->run();
    return pool;
}

template<typename T>
void thread_pool<T>::run()
{
    // 检查停止标记
    while (!m_stop)
    {
        m_queue_stat.wait();
        m_queue_locker.lock();
        if (m_workqueue.empty())
        {
            m_queue_locker.unlock();
            continue;
        }
        T* request = m_workqueue.front();
        m_workqueue.pop_front();

        m_queue_locker.unlock();
        if (request != nullptr)
        {
            request->process();
        }
    }
}





#endif