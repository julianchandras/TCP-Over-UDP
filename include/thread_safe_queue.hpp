#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>

template <typename T>
class ThreadSafeQueue
{
private:
    std::queue<T> queue;
    mutable std::mutex mtx;
    std::condition_variable cv;

public:
    void push(const T &item)
    {
        std::lock_guard<std::mutex> lock(mtx);
        queue.push(item);
        cv.notify_one();
    }

    T pop()
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return !queue.empty(); });
        T item = queue.front();
        queue.pop();
        return item;
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.empty();
    }

    size_t size() const
    {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.size();
    }
};

#endif
