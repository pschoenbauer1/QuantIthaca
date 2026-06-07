
#pragma once

#include <context/graph_key.h>
#include <context/graph_node.h>
#include <context/graph_value.h>

#include <string>

namespace graph
{
class DummyValue1 : public GraphValue
{
    double _x;

public:
    explicit DummyValue1(double x) : _x(x) {}

    double get_x() const { return _x; }

    static std::string name() { return "DummyValue1"; }
    std::string type_name() const override { return name(); }
};

class DummyGraphBuilder1 : public GraphBuilder
{
    DummyKey1 _key;
    KeySet _dependencies;

public:
    DummyGraphBuilder1(const DummyKey1&);
    GraphKey key() const override { return _key; }
    const KeySet& dependencies() const override { return _dependencies; }
    // CPtr<GraphValue> value(const Graph& graph) const override;
};

}  // namespace graph
