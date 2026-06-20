#pragma once

#include <context/graph_value.h>

#include <string>

#include "graph_key.h"

namespace graph
{

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

class DummyValuePy : public GraphValue
{
    double _value;

public:
    explicit DummyValuePy(double value) : _value(value) {}

    double value() const { return _value; }

    static std::string name() { return "DummyValuePy"; }
    std::string type_name() const override { return name(); }
};

}  // namespace graph
