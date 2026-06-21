#pragma once

#include <context/graph_value.h>

#include <string>

namespace graph
{

// Python-extensible value: builder class PyValueBuilder in graph.graph_obj_builders
struct PyKey
{
    std::string id = "node";

    static std::string name() { return "PyKey"; }
    std::string to_string() const { return "py_value:" + id; }
    auto to_tuple() const { return std::make_tuple(id); }
    auto operator<=>(const PyKey&) const = default;
};

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
