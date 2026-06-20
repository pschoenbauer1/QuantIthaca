#include "threads.h"

utils::ThreadPool::ThreadPool(int num_threads) : _num_threads(num_threads) {
    for (int i = 0; i < num_threads; ++i) {
        _threads.push_back(std::thread([this]() {
            while (!this->_stopped) {
                std::unique_lock<std::mutex> lock(this->_mutex);
                this->_cv.wait(lock, [this]() { return this->_stopped || !this->_queue.empty(); });
                if (this->_stopped) {
                    return;
                }
                const auto f = this->_queue.front();
                this->_queue.pop();
                ++_num_threads_working;
                lock.unlock();

                f();

                lock.lock();
                --_num_threads_working;
                lock.unlock();

                _cv.notify_one();
                _cv_all_done.notify_all();
            }
        }));
    }
}

utils::ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _stopped = true;
    }
    _cv.notify_all();
    _cv_all_done.notify_all();

    for (auto& thread : _threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}
