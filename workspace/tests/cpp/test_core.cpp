#include <gtest/gtest.h>

#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <unordered_map>
#include <vector>

#include "core.hpp"

TEST(CoreTest, AddWorks)
{
    EXPECT_EQ(qi::add(2, 3), 5);
}

TEST(CoreTest, DataStructures)
{
    std::vector<double> vec{1.0, 2.0};
    vec.push_back(3.0);
    vec.emplace_back(4.0);
    vec.pop_back();

    std::sort(vec.begin(), vec.end(), std::greater_equal<double>());

    double x_prev = std::numeric_limits<double>::infinity();
    for (const auto x : vec)
    {
        EXPECT_LE(x, x_prev);
        x_prev = x;
    }

    std::priority_queue<int> pq;
}

struct TestStruct
{
    double x = 0.5;
    double y = 3.14;

    auto operator<=>(const TestStruct& other) const = default;
};

TEST(CoreTest, Pointers)
{
    std::shared_ptr<TestStruct> ptr = std::make_shared<TestStruct>(TestStruct{.x = 5, .y = 10.});
    std::shared_ptr<TestStruct> ptr2 = std::make_shared<TestStruct>(TestStruct{.x = 5, .y = 10.});

    EXPECT_EQ(*ptr, *ptr2);
    EXPECT_NE(ptr, ptr2);

    ptr2 = ptr;

    EXPECT_EQ(ptr, ptr2);

    std::unique_ptr<TestStruct> ptr3 = std::make_unique<TestStruct>();
}