#pragma once

#include <context/graph_value.h>

#include <string>

namespace graph
{

// ---------- Key structs ----------

struct DummyKey1
{
    std::string symbol = "AAPL";

    static std::string name() { return "DummyKey1"; }
    std::string to_string() const { return "raw_price:" + symbol; }
    auto to_tuple() const { return std::make_tuple(symbol); }
    auto operator<=>(const DummyKey1&) const = default;
};

struct DummyKey2
{
    std::string index = "SOFR";

    static std::string name() { return "DummyKey2"; }
    std::string to_string() const { return "benchmark:" + index; }
    auto to_tuple() const { return std::make_tuple(index); }
    auto operator<=>(const DummyKey2&) const = default;
};

struct DummyKey3
{
    std::string symbol = "AAPL";

    static std::string name() { return "DummyKey3"; }
    std::string to_string() const { return "adjusted_price:" + symbol; }
    auto to_tuple() const { return std::make_tuple(symbol); }
    auto operator<=>(const DummyKey3&) const = default;
};

struct DummyKey4
{
    std::string symbol = "AAPL";
    std::string index = "SOFR";

    static std::string name() { return "DummyKey4"; }
    std::string to_string() const { return "spread:" + symbol + "/" + index; }
    auto to_tuple() const { return std::make_tuple(symbol, index); }
    auto operator<=>(const DummyKey4&) const = default;
};

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

// ---------- Value types ----------

class DummyValue1 : public GraphValue
{
    double _price;

public:
    explicit DummyValue1(double price) : _price(price) {}

    double price() const { return _price; }

    static std::string name() { return "DummyValue1"; }
    std::string type_name() const override { return name(); }
};

class DummyValue2 : public GraphValue
{
    double _rate;

public:
    explicit DummyValue2(double rate) : _rate(rate) {}

    double rate() const { return _rate; }

    static std::string name() { return "DummyValue2"; }
    std::string type_name() const override { return name(); }
};

class DummyValue3 : public GraphValue
{
    double _adjusted_price;

public:
    explicit DummyValue3(double adjusted_price) : _adjusted_price(adjusted_price) {}

    double adjusted_price() const { return _adjusted_price; }

    static std::string name() { return "DummyValue3"; }
    std::string type_name() const override { return name(); }
};

class DummyValue4 : public GraphValue
{
    double _spread;

public:
    explicit DummyValue4(double spread) : _spread(spread) {}

    double spread() const { return _spread; }

    static std::string name() { return "DummyValue4"; }
    std::string type_name() const override { return name(); }
};

class DummyValue5 : public GraphValue
{
    double _alpha;

public:
    explicit DummyValue5(double alpha) : _alpha(alpha) {}

    double alpha() const { return _alpha; }

    static std::string name() { return "DummyValue5"; }
    std::string type_name() const override { return name(); }
};

class DummyValuePy : public GraphValue
{
    double _value;

public:
    explicit DummyValuePy(double value) : _value(value) {}

    double value() const { return _value; }

    static std::string name() { return "DummyValuePy"; }
    std::string type_name() const override { return name(); }
};

// ---------- Builders ----------

class DummyGraphBuilder1 : public GraphBuilder
{
    DummyKey1 _key;
    KeySet _dependencies;

public:
    explicit DummyGraphBuilder1(const DummyKey1& key);
    GraphKey key() const override { return _key; }
    KeySet dependencies() const override { return _dependencies; }
};

class DummyGraphBuilder2 : public GraphBuilder
{
    DummyKey2 _key;
    KeySet _dependencies;

public:
    explicit DummyGraphBuilder2(const DummyKey2& key);
    GraphKey key() const override { return _key; }
    KeySet dependencies() const override { return _dependencies; }
};

class DummyGraphBuilder3 : public GraphBuilder
{
    DummyKey3 _key;
    KeySet _dependencies;

public:
    explicit DummyGraphBuilder3(const DummyKey3& key);
    GraphKey key() const override { return _key; }
    KeySet dependencies() const override { return _dependencies; }
    CPtr<GraphValue> value(const Graph& graph) const override;
};

class DummyGraphBuilder4 : public GraphBuilder
{
    DummyKey4 _key;
    KeySet _dependencies;

public:
    explicit DummyGraphBuilder4(const DummyKey4& key);
    GraphKey key() const override { return _key; }
    KeySet dependencies() const override { return _dependencies; }
    CPtr<GraphValue> value(const Graph& graph) const override;
};

class DummyGraphBuilder5 : public GraphBuilder
{
    DummyKey5 _key;
    KeySet _dependencies;

public:
    explicit DummyGraphBuilder5(const DummyKey5& key);
    GraphKey key() const override { return _key; }
    KeySet dependencies() const override { return _dependencies; }
    CPtr<GraphValue> value(const Graph& graph) const override;
};

class DummyKeyPyBuilder : public GraphBuilder
{
    DummyKeyPy _key;

public:
    explicit DummyKeyPyBuilder(const DummyKeyPy& key) : _key(key) {}
    GraphKey key() const override { return _key; }
    KeySet dependencies() const override { return {}; }
    CPtr<GraphValue> value(const Graph& graph) const override;
};

}  // namespace graph

// ---------- Mapping specializations ----------

template <>
struct graph::Mapping<graph::DummyKey1>
{
    using BuilderType = DummyGraphBuilder1;
    using ValueType   = DummyValue1;
};

template <>
struct graph::Mapping<graph::DummyKey2>
{
    using BuilderType = DummyGraphBuilder2;
    using ValueType   = DummyValue2;
};

template <>
struct graph::Mapping<graph::DummyKey3>
{
    using BuilderType = DummyGraphBuilder3;
    using ValueType   = DummyValue3;
};

template <>
struct graph::Mapping<graph::DummyKey4>
{
    using BuilderType = DummyGraphBuilder4;
    using ValueType   = DummyValue4;
};

template <>
struct graph::Mapping<graph::DummyKey5>
{
    using BuilderType = DummyGraphBuilder5;
    using ValueType   = DummyValue5;
};

template <>
struct graph::Mapping<graph::DummyKeyPy>
{
    using BuilderType = DummyKeyPyBuilder;
    using ValueType   = DummyValuePy;
};
