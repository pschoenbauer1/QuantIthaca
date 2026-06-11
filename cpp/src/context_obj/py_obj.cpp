
#include "py_obj.h"

#include <context/graph_value.h>
#include <context_obj/graph_key.h>

namespace graph
{

template <>
CPtr<GraphBuilder> make_builder(const PyKey& key)
{
    return make_py_builder(PyKey::value_type_name(), GraphKey(key));
}

}  // namespace graph
