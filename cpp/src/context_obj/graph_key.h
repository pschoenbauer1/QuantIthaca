#pragma once

#include <utils/hash.h>

#include <concepts>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>

namespace graph
{

class DummyValue1;
class DummyValue2;
class DummyValue3;
class DummyValue4;
class DummyValue5;
class DummyValuePy;

// 1. Raw market price (leaf input)
struct DummyKey1
{
    std::string symbol = "AAPL";

    using ValueType = DummyValue1;
    static std::string name() { return "DummyKey1"; }
    static std::string value_type_name() { return "DummyValue1"; }
    std::string to_string() const { return "raw_price:" + symbol; }
    auto to_tuple() const { return std::make_tuple(symbol); }
    auto operator<=>(const DummyKey1&) const = default;
};

// 2. Benchmark rate (leaf input)
struct DummyKey2
{
    std::string index = "SOFR";

    using ValueType = DummyValue2;
    static std::string name() { return "DummyKey2"; }
    static std::string value_type_name() { return "DummyValue2"; }
    std::string to_string() const { return "benchmark:" + index; }
    auto to_tuple() const { return std::make_tuple(index); }
    auto operator<=>(const DummyKey2&) const = default;
};

// 3. Adjusted price = raw price * markup (depends on 1)
struct DummyKey3
{
    std::string symbol = "AAPL";

    using ValueType = DummyValue3;
    static std::string name() { return "DummyKey3"; }
    static std::string value_type_name() { return "DummyValue3"; }
    std::string to_string() const { return "adjusted_price:" + symbol; }
    auto to_tuple() const { return std::make_tuple(symbol); }
    auto operator<=>(const DummyKey3&) const = default;
};

// 4. Spread = adjusted price - benchmark (depends on 3, 2)
struct DummyKey4
{
    std::string symbol = "AAPL";
    std::string index = "SOFR";

    using ValueType = DummyValue4;
    static std::string name() { return "DummyKey4"; }
    static std::string value_type_name() { return "DummyValue4"; }
    std::string to_string() const { return "spread:" + symbol + "/" + index; }
    auto to_tuple() const { return std::make_tuple(symbol, index); }
    auto operator<=>(const DummyKey4&) const = default;
};

// 5. Alpha = spread * adjusted / benchmark (depends on 4, 3, 2)
struct DummyKey5
{
    std::string symbol = "AAPL";
    std::string index = "SOFR";

    using ValueType = DummyValue5;
    static std::string name() { return "DummyKey5"; }
    static std::string value_type_name() { return "DummyValue5"; }
    std::string to_string() const { return "alpha:" + symbol + "/" + index; }
    auto to_tuple() const { return std::make_tuple(symbol, index); }
    auto operator<=>(const DummyKey5&) const = default;
};

// Python-backed node: builder class DummyValuePy in graph.graph_obj_builders
struct DummyKeyPy
{
    int x = 1;
    int y = 1;

    using ValueType = DummyValuePy;
    static std::string name() { return "DummyKeyPy"; }
    static std::string value_type_name() { return "DummyValuePy"; }
    std::string to_string() const { return "py:" + std::to_string(x) + "/" + std::to_string(y); }
    auto to_tuple() const { return std::make_tuple(x, y); }
    auto operator<=>(const DummyKeyPy&) const = default;
};

using GraphKey =
    std::variant<DummyKey1, DummyKey2, DummyKey3, DummyKey4, DummyKey5, DummyKeyPy>;

template <typename T>
using KeyMap = std::unordered_map<GraphKey, T, utils::TupleHash>;
using KeySet = std::unordered_set<GraphKey, utils::TupleHash>;

inline std::string to_string(const GraphKey& key)
{
    return std::visit([](const auto& key_) { return key_.to_string(); }, key);
}

}  // namespace graph
