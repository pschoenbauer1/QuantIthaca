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
#include "sort.h"
#include "threads.h"

TEST(GraphTest, TestConstruction) {
    graph::DummyKey1 key1a{.idx = 10};
    graph::DummyKey1 key1b{.idx = 20};
    graph::DummyKey2 key2a{.str = "ciao"};

    graph::Graph ctxt;

    ctxt.set_value(key1a, std::make_shared<graph::DummyValue1>(3.14));
    ctxt.set_value(key1b, std::make_shared<graph::DummyValue1>(-3.14));
    ctxt.set_value(key2a, std::make_shared<graph::DummyValue2>(1000.1));

    std::cout << ctxt.value(key1a)->get_x() << std::endl;
    std::cout << ctxt.value(key1b)->get_x() << std::endl;
}