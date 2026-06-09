#include <context/graph.h>
#include <context_obj/dummy_obj.h>
#include <gtest/gtest.h>
#include <py/py_runtime.h>

#include <memory>
#include <stdexcept>

namespace
{
constexpr double k_price_markup = 1.05;

const graph::DummyKey1 k_raw_price_key{.symbol = "AAPL"};
const graph::DummyKey2 k_benchmark_key{.index = "SOFR"};
const graph::DummyKey3 k_adjusted_price_key{.symbol = "AAPL"};
const graph::DummyKey4 k_spread_key{.symbol = "AAPL", .index = "SOFR"};
const graph::DummyKey5 k_alpha_key{.symbol = "AAPL", .index = "SOFR"};
}  // namespace

TEST(GraphTest, EmptyGraphStaysEmptyAfterCompute)
{
    graph::Graph graph;

    EXPECT_TRUE(graph.empty());
    EXPECT_TRUE(graph.keys().empty());
    EXPECT_NO_THROW(graph.compute());
    EXPECT_TRUE(graph.empty());
    EXPECT_TRUE(graph.keys().empty());
}

TEST(GraphTest, InsertDummy1KeyOnly)
{
    graph::Graph graph;

    graph.insert(k_raw_price_key);

    EXPECT_FALSE(graph.empty());
    EXPECT_TRUE(graph.contains(k_raw_price_key));
    EXPECT_TRUE(graph.contains(graph::GraphKey(k_raw_price_key)));
    EXPECT_EQ(graph.keys().size(), 1u);
    EXPECT_NO_THROW(graph.compute());
    EXPECT_TRUE(graph.contains(k_raw_price_key));
    EXPECT_THROW(graph.get_value(k_raw_price_key), std::runtime_error);
}

TEST(GraphTest, InsertDummy1WithValue)
{
    graph::Graph graph;

    graph.insert(k_raw_price_key, std::make_shared<graph::DummyValue1>(42.0));

    EXPECT_TRUE(graph.contains(k_raw_price_key));
    EXPECT_DOUBLE_EQ(graph.get_value(k_raw_price_key)->price(), 42.0);
    EXPECT_NO_THROW(graph.compute());
    EXPECT_DOUBLE_EQ(graph.get_value(k_raw_price_key)->price(), 42.0);
}

TEST(GraphTest, KeysTracksMultipleInserts)
{
    graph::Graph graph;

    graph.insert(k_raw_price_key);
    graph.insert(k_benchmark_key);

    EXPECT_EQ(graph.keys().size(), 2u);
    EXPECT_TRUE(graph.contains(k_raw_price_key));
    EXPECT_TRUE(graph.contains(k_benchmark_key));
    EXPECT_FALSE(graph.contains(k_adjusted_price_key));
}

TEST(GraphTest, ComputePullsInDependencies)
{
    graph::Graph graph;

    graph.insert(k_raw_price_key, std::make_shared<graph::DummyValue1>(100.0));
    graph.insert(k_benchmark_key, std::make_shared<graph::DummyValue2>(102.0));
    graph.insert(k_alpha_key);

    EXPECT_EQ(graph.keys().size(), 3u);

    graph.compute();

    EXPECT_TRUE(graph.contains(k_adjusted_price_key));
    EXPECT_TRUE(graph.contains(k_spread_key));
    EXPECT_EQ(graph.keys().size(), 5u);
}

TEST(GraphTest, ComputesDummyPyNode)
{
    py_runtime::ensure_core_bind();

    const graph::DummyKeyPy key{.x = 10, .y = 4};
    graph::Graph graph;

    graph.insert(key);
    graph.compute();

    EXPECT_TRUE(graph.contains(key));
    EXPECT_DOUBLE_EQ(graph.get_value(key)->value(), 2.5);
}

TEST(GraphTest, ComputesPricingPipeline)
{
    // Example: price AAPL vs SOFR benchmark, then derive spread and alpha.
    //
    //   raw_price (1)     benchmark (2)
    //        |                  |
    //        v                  |
    //   adjusted_price (3)       |
    //        +-----------------+
    //        |
    //        v
    //      spread (4)
    //        +--------+---------+
    //        |        |         |
    //        v        v         v
    //      alpha (5)  3         2

    graph::Graph graph;

    constexpr double k_raw_price = 100.0;
    constexpr double k_benchmark = 102.0;
    constexpr double k_adjusted = k_raw_price * k_price_markup;
    constexpr double k_spread = k_adjusted - k_benchmark;
    constexpr double k_alpha = k_spread * k_adjusted / k_benchmark;

    graph.insert(k_raw_price_key, std::make_shared<graph::DummyValue1>(k_raw_price));
    graph.insert(k_benchmark_key, std::make_shared<graph::DummyValue2>(k_benchmark));
    graph.insert(k_alpha_key);

    graph.compute();

    EXPECT_EQ(graph.keys().size(), 5u);
    EXPECT_TRUE(graph.contains(k_raw_price_key));
    EXPECT_TRUE(graph.contains(k_benchmark_key));
    EXPECT_TRUE(graph.contains(k_adjusted_price_key));
    EXPECT_TRUE(graph.contains(k_spread_key));
    EXPECT_TRUE(graph.contains(k_alpha_key));

    EXPECT_DOUBLE_EQ(graph.get_value(k_raw_price_key)->price(), k_raw_price);
    EXPECT_DOUBLE_EQ(graph.get_value(k_benchmark_key)->rate(), k_benchmark);
    EXPECT_DOUBLE_EQ(graph.get_value(k_adjusted_price_key)->adjusted_price(), k_adjusted);
    EXPECT_DOUBLE_EQ(graph.get_value(k_spread_key)->spread(), k_spread);
    EXPECT_DOUBLE_EQ(graph.get_value(k_alpha_key)->alpha(), k_alpha);
}
