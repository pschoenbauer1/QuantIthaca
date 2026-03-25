#include <gtest/gtest.h>

#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <unordered_map>
#include <vector>

#include "core.hpp"
#include "threads.h"

TEST(UtilsTest, TestThreadPool) {
    utils::ThreadPool p(4);

    int x = 2;

    p.push([&]() { x = 4; });

    p.wait();

    EXPECT_EQ(x, 4);
}