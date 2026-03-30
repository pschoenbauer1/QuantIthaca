#include <gtest/gtest.h>
#include <math.h>

#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <unordered_map>
#include <vector>

#include "core.hpp"
#include "graph.h"
#include "heap.h"
#include "py_calls.h"
#include "sort.h"
#include "threads.h"

TEST(UtilsTest, TestThreadPool) {
    utils::ThreadPool p(4);

    int x = 2;

    p.push([&]() { x = 4; });
    auto future = p.push([&]() {
        double y = std::exp(.123);
        return y;
    });

    double v = future.get();

    EXPECT_DOUBLE_EQ(v, std::exp(.123));

    p.wait();

    EXPECT_EQ(x, 4);
}

TEST(UtilsTest, TestQuickSort) {
    std::vector<double> vec{1., 2., 3., 0., 4.};

    const auto vec_sorted = utils::quick_sort(vec);

    EXPECT_TRUE(utils::is_sorted(vec_sorted));

    for (const auto x : vec_sorted) {
        std::cout << x << ", ";
    }
    std::cout << std::endl;
}

TEST(UtilsTest, TestHeap) {
    utils::Heap<double> heap;
    heap.push(1.0);
    heap.push(2.0);
    heap.push(0.0);
    heap.push(1.5);
    heap.push(-100.);
    heap.push(0.0);
    heap.push(-100.);

    EXPECT_EQ(heap.top(), 2.0);
    heap.pop();
    EXPECT_EQ(heap.top(), 1.5);
    heap.pop();
    EXPECT_EQ(heap.top(), 1.0);
    heap.pop();
    EXPECT_EQ(heap.top(), 0.0);
    heap.pop();
    EXPECT_EQ(heap.top(), 0.0);
    heap.pop();
    EXPECT_EQ(heap.top(), -100.0);
    heap.pop();
    EXPECT_EQ(heap.top(), -100.0);
}

TEST(TestCallback, TestCallback) {
    std::string callbackstr = callback::test_callback();
    EXPECT_EQ(callbackstr, "Hello C++! Python Here!");
}