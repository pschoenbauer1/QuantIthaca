#include <context/graph.h>
#include <context_obj/dummy_obj1.h>
#include <gtest/gtest.h>
#include <math.h>
#include <utils/heap.h>
#include <utils/sort.h>
#include <utils/threads.h>

#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <unordered_map>
#include <vector>

#include "core.hpp"

TEST(GraphTest, TestConstruction)
{
    graph::DummyKey1 key1a{.idx = 10};
    graph::DummyKey1 key1b{.idx = 20};
    graph::DummyKey2 key2a{.str = "ciao"};

    graph::Graph ctxt;

    const auto value1 = std::make_shared<graph::DummyValue1>(3.14);
    ctxt.insert(key1a, value1);
    ctxt.insert(key1b, std::make_shared<graph::DummyValue1>(-3.14));
    ctxt.insert(key2a, std::make_shared<graph::DummyValue2>(1000.1));

    // std::cout << ctxt.value(key1a)->get_x() << std::endl;
    // std::cout << ctxt.value(key1b)->get_x() << std::endl;
}