#pragma once

#include <context/graph_value.h>

#include <string>

namespace graph
{

class PyValue : public GraphValue
{
public:
    ~PyValue() override = default;

    static std::string name() { return "PyValue"; }
    std::string type_name() const override { return name(); }
};

class PyKeyGraphBuilder : public GraphBuilder
{
    PyKey _key;
    KeySet _dependencies;

public:
    explicit PyKeyGraphBuilder(const PyKey& key);
    GraphKey key() const override { return _key; }
    KeySet dependencies() const override { return _dependencies; }
    CPtr<GraphValue> value(const Graph& graph) const override;
};

}  // namespace graph
