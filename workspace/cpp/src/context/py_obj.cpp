
#include "py_obj.h"

#include <context/graph.h>

namespace graph
{

PyKeyGraphBuilder::PyKeyGraphBuilder(const PyKey& key) : _key(key) {}

CPtr<GraphValue> PyKeyGraphBuilder::value(const Graph& graph) const
{
    return make_py_builder(PyValue::name(), GraphKey(_key))->value(graph);
}

}  // namespace graph
