#pragma once

#include <utils/exception.h>

#include <atomic>
#include <deque>
#include <iostream>
#include <memory>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

namespace utils
{

template <typename T>
class SpscRingBuffer
{
    int _buffer_size{};
    std::atomic<int> _head{};
    std::atomic<int> _tail{};
    std::vector<T> _buffer{};

public:
    explicit SpscRingBuffer(int buffer_size)
        : _buffer_size(PS_VALIDATE_POSITIVE(buffer_size)),
          _head(0),
          _tail(0),
          _buffer(std::vector<T>(buffer_size))
    {
    }

    bool push(const T& t)
    {
        int tail = _tail.load(std::memory_order_relaxed);
        int tail_next = (tail + 1) % _buffer_size;

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
        int head_next = (head + 1) % _buffer_size;

        if (head == tail)
        {
            return false;
        }

        t = std::move(_buffer[head]);

        _head.store(head_next, std::memory_order_release);

        return true;
    }
};

}  // namespace utils