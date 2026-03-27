
#pragma once

#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <thread>

namespace utils {

using Mutex = std::shared_mutex;
using ReadLock = std::shared_lock<std::shared_mutex>;
using WriteLock = std::unique_lock<std::shared_mutex>;

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
    auto push(F&& f) -> std::future<decltype(f())> {
        using ReturnType = decltype(f());
        const auto promise = std::make_shared<std::promise<ReturnType>>();
        const auto f_wrapped = [this, &f, promise]() {
            if constexpr (std::is_void_v<ReturnType>) {
                f();
                promise->set_value();
            } else {
                auto value = f();
                promise->set_value(std::move(value));
            }
        };
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _queue.push(f_wrapped);
        }
        _cv.notify_one();
        return promise->get_future();
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