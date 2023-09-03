#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <arpa/inet.h>
#include <functional>
#include <cassert>
#include <chrono>

using Clock = std::chrono::high_resolution_clock;
using TimeStamp = Clock::time_point;
using TimeoutCallBack = std::function<void()>;
using MS = std::chrono::milliseconds;

struct TimerNode
{
    int id;
    TimeStamp expires;
    TimeoutCallBack call_back_fun;

    TimerNode() = default;

    TimerNode(int id, TimeStamp expires, TimeoutCallBack call_back_fun) : id(id), expires(expires), call_back_fun(call_back_fun) {}

    bool operator<(const TimerNode &t)
    {
        return expires < t.expires;
    }
};
class Timer
{
public:
    Timer()
    {
        // heap_.reserve(64);
        heap_.emplace_back(-1, TimeStamp(), []() {});
    }

    ~Timer()
    {
        Clear();
    }

    void Adjust(int id, int newExpires);
    void Add(int id, int timeOut, const TimeoutCallBack &call_back_fun);
    void DoWork(int id);
    void Clear();
    void Tick();
    void Pop();
    int GetNextTick();

private:
    void Del_(int i);
    void Shiftup_(int i);
    void Shiftdown_(int index, int n);
    void SwapNode_(int i, int j);

    std::vector<TimerNode> heap_;
    std::unordered_map<int, int> ref_;
};

#endif // HEAP_TIMER_H