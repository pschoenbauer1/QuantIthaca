
#pragma once

#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

namespace utils {
class ThreadPool {
    int _num_threads;
    std::queue<std::function<void()>> _queue;
    std::vector<std::thread> _threads;
    bool _stopped = false;
    std::mutex _mutex;
    std::condition_variable _cv;
    std::condition_variable _cv_all_done;
    int _num_threads_working = 0;

   public:
    ThreadPool(int num_threads);
    ~ThreadPool();

    template <typename F>
    void push(F&& f) {
        _queue.push(f);
        _cv.notify_one();
    }

    void wait() {
        std::unique_lock<std::mutex> lock(_mutex);
        if (_queue.empty()) {
            return;
        }
        _cv_all_done.wait(lock, [this]() {
            return this->_stopped || (_num_threads_working == 0 && this->_queue.empty());
        });
        lock.unlock();
    }
};

}  // namespace utils