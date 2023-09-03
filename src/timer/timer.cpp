#include "timer.h"

void Timer::Add(int id, int timeout, const TimeoutCallBack &call_back_fun)
{
    assert(id >= 0);
    if (ref_.count(id) == 0)
    {
        // 新节点插入堆尾，调整堆
        int index = heap_.size();
        ref_[id] = index;
        heap_.push_back({id, Clock::now() + MS(timeout), call_back_fun});
        Shiftup_(index);
    }
    else
    {
        // 已存在结点则调整堆
        int index = ref_[id];
        heap_[index].expires = Clock::now() + MS(timeout);
        heap_[index].call_back_fun = call_back_fun;
        Shiftdown_(index, heap_.size());
        Shiftup_(index);
    }
}

void Timer::DoWork(int id)
{
    // 删除指定id结点，触发回调函数
    if (heap_.size() <= 1 || ref_.count(id) == 0)
    {
        return;
    }
    int index = ref_[id];
    TimerNode node = heap_[index];
    node.call_back_fun();
    Del_(index);
}

void Timer::Del_(int index)
{
    assert(heap_.size() > 1 && index >= 1 && index < heap_.size());
    // 将要删除的结点换到队尾，然后调整堆
    int n = heap_.size() - 1;
    assert(index <= n);
    if (index < n)
    {
        SwapNode_(index, n);
        Shiftdown_(index, n);
        Shiftup_(index);
    }
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}

void Timer::Adjust(int id, int timeout)
{
    // 调整指定的结点
    assert(heap_.size() > 1);
    assert(ref_.count(id) > 0);
    heap_[ref_[id]].expires = Clock::now() + MS(timeout);
    Shiftdown_(ref_[id], heap_.size());
}

void Timer::Tick()
{
    if (heap_.size() <= 1)
    {
        return;
    }
    while (heap_.size() > 1)
    {
        TimerNode node = heap_[1];
        if (std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0)
        {
            break;
        }
        node.call_back_fun();
        Pop();
    }
}

void Timer::Pop()
{
    assert(heap_.size() > 1);
    Del_(1);
}

void Timer::Clear()
{
    ref_.clear();
    heap_.clear();
    heap_.emplace_back(-1, TimeStamp(), []() {});
}

int Timer::GetNextTick()
{
    Tick();
    int res = -1;
    if (heap_.size() > 1)
    {
        res = std::chrono::duration_cast<MS>(heap_[1].expires - Clock::now()).count();
        if (res < 0)
        {
            res = 0;
        }
    }
    return res;
}

void Timer::Shiftup_(int index)
{
    while (index > 1)
    {
        int fa = index >> 1;
        if (heap_[fa] < heap_[index])
            break;
        SwapNode_(index, fa);
        index = fa;
    }
}

void Timer::SwapNode_(int i, int j)
{
    if (i == j)
        return;
    assert(i >= 1 && i < heap_.size());
    assert(j >= 1 && j < heap_.size());
    std::swap(heap_[i], heap_[j]);
    ref_[heap_[i].id] = i;
    ref_[heap_[j].id] = j;
}

void Timer::Shiftdown_(int index, int n)
{
    assert(index >= 1);
    while (index < n)
    {
        int son = index << 1;
        if (son + 1 < n && heap_[son + 1] < heap_[son])
            ++son;
        if (son >= n)
            return;
        if (heap_[index] < heap_[son])
            break;
        SwapNode_(index, son);
        index = son;
    }
}