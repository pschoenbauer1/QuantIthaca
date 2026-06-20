#pragma once

#include <context/graph_value.h>
#include <context_obj/key_model.h>

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

template <>
struct graph::Mapping<graph::PyKey>
{
    using BuilderType = PyKeyGraphBuilder;
    using ValueType   = PyValue;
};

// PyKey nodes hold arbitrary Python-defined values (PyValue subclasses).
template <>
inline bool graph::KeyModel<graph::PyKey>::allows_py_value_subclasses() const
{
    return true;
}
