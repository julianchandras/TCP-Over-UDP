#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

template <typename T>
class ThreadSafeQueue {
private:
    std::queue<T> queue;
    mutable std::mutex mtx;
    std::condition_variable cv;

public:
    void push(const T &item) {
        std::lock_guard<std::mutex> lock(mtx);
        queue.push(item);
        cv.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return !queue.empty(); }); // Wait until the queue is not empty
        T item = queue.front();
        queue.pop();
        return item;
    }

    // Attempt to pop an item without waiting
    std::optional<T> tryPop() {
        std::lock_guard<std::mutex> lock(mtx);
        if (queue.empty()) {
            return std::nullopt; // No item available
        }
        T item = queue.front();
        queue.pop();
        return item;
    }

    // Check if the queue is empty
    bool empty() const {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.empty();
    }

    // Get the current size of the queue
    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.size();
    }
};
