#pragma once

#include <context_obj/i_graph_key.h>
#include <context_obj/key_model.h>
#include <utils/hash.h>

#include <concepts>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace graph
{

// ---------- KeyLike concept (moved here from graph.h) ----------

template <typename T>
concept KeyLike = requires(const T t) {
    { T::name() } -> std::convertible_to<std::string>;
    { t.to_string() } -> std::convertible_to<std::string>;
    { t.to_tuple() };
};

// ---------- Key struct forward declarations ----------

class DummyValue1;
class DummyValue2;
class DummyValue3;
class DummyValue4;
class DummyValue5;
class DummyValuePy;
class PyValue;

// 1. Raw market price (leaf input)
struct DummyKey1
{
    std::string symbol = "AAPL";

    static std::string name() { return "DummyKey1"; }
    std::string to_string() const { return "raw_price:" + symbol; }
    auto to_tuple() const { return std::make_tuple(symbol); }
    auto operator<=>(const DummyKey1&) const = default;
};

// 2. Benchmark rate (leaf input)
struct DummyKey2
{
    std::string index = "SOFR";

    static std::string name() { return "DummyKey2"; }
    std::string to_string() const { return "benchmark:" + index; }
    auto to_tuple() const { return std::make_tuple(index); }
    auto operator<=>(const DummyKey2&) const = default;
};

// 3. Adjusted price = raw price * markup (depends on 1)
struct DummyKey3
{
    std::string symbol = "AAPL";

    static std::string name() { return "DummyKey3"; }
    std::string to_string() const { return "adjusted_price:" + symbol; }
    auto to_tuple() const { return std::make_tuple(symbol); }
    auto operator<=>(const DummyKey3&) const = default;
};

// 4. Spread = adjusted price - benchmark (depends on 3, 2)
struct DummyKey4
{
    std::string symbol = "AAPL";
    std::string index = "SOFR";

    static std::string name() { return "DummyKey4"; }
    std::string to_string() const { return "spread:" + symbol + "/" + index; }
    auto to_tuple() const { return std::make_tuple(symbol, index); }
    auto operator<=>(const DummyKey4&) const = default;
};

// 5. Alpha = spread * adjusted / benchmark (depends on 4, 3, 2)
struct DummyKey5
{
    std::string symbol = "AAPL";
    std::string index = "SOFR";

    static std::string name() { return "DummyKey5"; }
    std::string to_string() const { return "alpha:" + symbol + "/" + index; }
    auto to_tuple() const { return std::make_tuple(symbol, index); }
    auto operator<=>(const DummyKey5&) const = default;
};

// Python-backed node: builder class DummyValuePyBuilder in graph.graph_obj_builders
struct DummyKeyPy
{
    int x = 1;
    int y = 1;

    static std::string name() { return "DummyKeyPy"; }
    std::string to_string() const { return "py:" + std::to_string(x) + "/" + std::to_string(y); }
    auto to_tuple() const { return std::make_tuple(x, y); }
    auto operator<=>(const DummyKeyPy&) const = default;
};

// Python-extensible value: builder class PyValueBuilder in graph.graph_obj_builders
struct PyKey
{
    std::string id = "node";

    static std::string name() { return "PyKey"; }
    std::string to_string() const { return "py_value:" + id; }
    auto to_tuple() const { return std::make_tuple(id); }
    auto operator<=>(const PyKey&) const = default;
};

// ---------- GraphKey: open, type-erased key ----------

class GraphKey
{
    std::shared_ptr<const IGraphKey> _impl;

public:
    template <typename K>
        requires KeyLike<K>
    explicit(false) GraphKey(K k) : _impl(std::make_shared<KeyModel<K>>(std::move(k)))
    {}

    std::string name() const { return _impl->name(); }
    std::string to_string() const { return _impl->to_string(); }
    std::string value_type_name() const { return _impl->value_type_name(); }
    bool allows_py_value_subclasses() const { return _impl->allows_py_value_subclasses(); }
    std::type_index type_index() const { return _impl->type_index(); }
    CPtr<GraphBuilder> make_builder() const { return _impl->make_builder(); }
    const IGraphKey& impl() const { return *_impl; }

    bool operator==(const GraphKey& other) const { return _impl->equals(*other._impl); }
};

template <typename T>
using KeyMap = std::unordered_map<GraphKey, T>;
using KeySet = std::unordered_set<GraphKey>;

inline std::string to_string(const GraphKey& key)
{
    return key.to_string();
}

}  // namespace graph

template <>
struct std::hash<graph::GraphKey>
{
    std::size_t operator()(const graph::GraphKey& k) const noexcept
    {
        return k.impl().hash();
    }
};
