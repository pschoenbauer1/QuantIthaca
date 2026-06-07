#pragma once

#include <utils/exception.h>

#include <atomic>
#include <deque>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace utils
{

template <typename T>
class SpscRingBuffer
{
    int _capacity{};
    std::atomic<int> _head{};
    std::atomic<int> _tail{};
    std::vector<T> _buffer{};

public:
    explicit SpscRingBuffer(int capacity)
        : _capacity(PS_VALIDATE_POSITIVE(capacity)),
          _head(0),
          _tail(0),
          _buffer(std::vector<T>(capacity))
    {
        bool is_power_of_2 = (_capacity & (_capacity - 1)) == 0;
        PS_EXPECT_THAT(is_power_of_2);
    }

    bool push(const T& t)
    {
        int tail = _tail.load(std::memory_order_relaxed);
        int tail_next = (tail + 1) & (_capacity - 1);

        int head = _head.load(std::memory_order_acquire);

        if (head == tail_next)
        {
            return false;
        }

        _buffer[tail] = t;

        _tail.store(tail_next, std::memory_order_release);

        return true;
    }

    bool pop(T& t)
    {
        int head = _head.load(std::memory_order_acquire);
        int tail = _tail.load(std::memory_order_acquire);
        int head_next = (head + 1) & (_capacity - 1);

        if (head == tail)
        {
            return false;
        }

        t = std::move(_buffer[head]);

        _head.store(head_next, std::memory_order_release);

        return true;
    }
};

template <typename T>
class RingBuffer
{
    int _buffer_size{};
    int _head{};
    int _tail{};
    std::vector<T> _buffer{};

    std::shared_mutex _mutex;

public:
    explicit RingBuffer(int buffer_size)
        : _buffer_size(PS_VALIDATE_POSITIVE(buffer_size)),
          _head(0),
          _tail(0),
          _buffer(std::vector<T>(buffer_size))
    {
    }

    bool push(const T& t)
    {
        std::unique_lock<std::shared_mutex> lock(_mutex);

        int tail_next = (_tail + 1) % _buffer_size;

        if (_head == tail_next)
        {
            return false;
        }

        _buffer[_tail] = t;

        _tail = tail_next;

        return true;
    }

    bool pop(T& t)
    {
        std::unique_lock<std::shared_mutex> lock(_mutex);

        if (_head == _tail)
        {
            return false;
        }

        t = std::move(_buffer[_head]);
        _head = (_head + 1) % _buffer_size;

        return true;
    }
};

}  // namespace utils