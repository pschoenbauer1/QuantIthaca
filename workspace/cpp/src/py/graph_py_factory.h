#pragma once

#include <context/graph_value.h>
#include <context_obj/i_graph_key.h>

#include <functional>
#include <string>

#if defined(PY_BRIDGE_USE_PYBIND)
#include <pybind11/pybind11.h>
namespace py = pybind11;
namespace graph
{
using PyToPyFn  = std::function<py::object(const IGraphKey&)>;
using PyFromFn  = std::function<GraphKey(py::object)>;
void register_pybind_key_converter(const char* type_id_name,
                                   const char* py_class_name,
                                   PyToPyFn   to_py,
                                   PyFromFn   from_py);
py::object graph_key_to_python_pb(const GraphKey& key);
GraphKey   python_to_graph_key_pb(py::object obj);
}  // namespace graph
#else
#include <nanobind/nanobind.h>
namespace nb = nanobind;
namespace graph
{
using NbToPyFn  = std::function<nb::object(const IGraphKey&)>;
using NbFromFn  = std::function<GraphKey(nb::handle)>;
void register_nb_key_converter(const char* type_id_name,
                               const char* py_class_name,
                               NbToPyFn   to_py,
                               NbFromFn   from_py);
nb::object graph_key_to_python(const GraphKey& key);
GraphKey   python_to_graph_key(nb::handle h);
}  // namespace graph
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
