
#include "py_obj.h"

#include <context/graph.h>
#include <context/graph_value.h>
#include <context_obj/graph_key.h>

namespace graph
{

PyKeyGraphBuilder::PyKeyGraphBuilder(const PyKey& key) : _key(key) {}

CPtr<GraphValue> PyKeyGraphBuilder::value(const Graph& graph) const
{
    return make_py_builder(PyValue::name(), GraphKey(_key))->value(graph);
}

}  // namespace graph
