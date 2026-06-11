#pragma once

#include <context/graph_value.h>

#if !defined(PY_BRIDGE_USE_PYBIND)
namespace nanobind
{
class handle;
}
#endif

namespace graph
{

// Register the default Python GraphBuilder factory for this module (exe or pyd).
void install_py_builder_factory();

// Register the batch Python leaf-node compute hook (one C++ -> Python call per compute()).
void install_py_batch_compute();

#if !defined(PY_BRIDGE_USE_PYBIND)
CPtr<GraphValue> nanobind_python_to_graph_value(nanobind::handle value);
#endif

}  // namespace graph
