#include <context/graph.h>
#include <context_obj/dummy_obj.h>
#include <context_obj/graph_key.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>

#include <vector>

#include "core.hpp"
#include "graph_builder_trampoline.h"
#include "py_bridge.h"

namespace nb = nanobind;
using namespace nb::literals;

namespace
{
std::vector<graph::GraphKey> key_set_to_vector(const graph::KeySet& keys)
{
    std::vector<graph::GraphKey> result;
    result.reserve(keys.size());
    for (const auto& key : keys)
    {
        result.push_back(key);
    }
    return result;
}
}  // namespace

NB_MODULE(core_bind, m)
{
    m.doc() = "QuantIthaca native module";
    m.def("add", &qi::add, "Add two integers");
    m.def("cpp_callback", [&](const std::string& py_func, const std::string& arg)
          { return py_bridge::call_python<std::string>(py_func, arg); });
    m.def("graph_key_to_string", &graph::to_string, "key"_a);

    // Graph Keys

    nb::class_<graph::DummyKey1>(m, "DummyKey1")
        .def(nb::init<>())
        .def(nb::init<std::string>(), "symbol"_a)
        .def_rw("symbol", &graph::DummyKey1::symbol)
        .def("__repr__", [](const graph::DummyKey1& key) { return key.to_string(); });

    nb::class_<graph::DummyKey2>(m, "DummyKey2")
        .def(nb::init<>())
        .def(nb::init<std::string>(), "index"_a)
        .def_rw("index", &graph::DummyKey2::index)
        .def("__repr__", [](const graph::DummyKey2& key) { return key.to_string(); });

    nb::class_<graph::DummyKey3>(m, "DummyKey3")
        .def(nb::init<>())
        .def(nb::init<std::string>(), "symbol"_a)
        .def_rw("symbol", &graph::DummyKey3::symbol)
        .def("__repr__", [](const graph::DummyKey3& key) { return key.to_string(); });

    nb::class_<graph::DummyKey4>(m, "DummyKey4")
        .def(nb::init<>())
        .def(nb::init<std::string, std::string>(), "symbol"_a, "index"_a)
        .def_rw("symbol", &graph::DummyKey4::symbol)
        .def_rw("index", &graph::DummyKey4::index)
        .def("__repr__", [](const graph::DummyKey4& key) { return key.to_string(); });

    nb::class_<graph::DummyKey5>(m, "DummyKey5")
        .def(nb::init<>())
        .def(nb::init<std::string, std::string>(), "symbol"_a, "index"_a)
        .def_rw("symbol", &graph::DummyKey5::symbol)
        .def_rw("index", &graph::DummyKey5::index)
        .def("__repr__", [](const graph::DummyKey5& key) { return key.to_string(); });

    nb::class_<graph::DummyKeyPy>(m, "DummyKeyPy")
        .def(nb::init<>())
        .def(nb::init<int, int>(), "x"_a, "y"_a)
        .def_rw("x", &graph::DummyKeyPy::x)
        .def_rw("y", &graph::DummyKeyPy::y)
        .def("__repr__", [](const graph::DummyKeyPy& key) { return key.to_string(); });

    // Graph Values

    nb::class_<graph::GraphValue>(m, "GraphValue")
        .def("type_name", &graph::GraphValue::type_name);

    nb::class_<graph::DummyValue1, graph::GraphValue>(m, "DummyValue1")
        .def(nb::init<double>(), "price"_a)
        .def("price", &graph::DummyValue1::price);

    nb::class_<graph::DummyValue2, graph::GraphValue>(m, "DummyValue2")
        .def(nb::init<double>(), "rate"_a)
        .def("rate", &graph::DummyValue2::rate);

    nb::class_<graph::DummyValue3, graph::GraphValue>(m, "DummyValue3")
        .def(nb::init<double>(), "adjusted_price"_a)
        .def("adjusted_price", &graph::DummyValue3::adjusted_price);

    nb::class_<graph::DummyValue4, graph::GraphValue>(m, "DummyValue4")
        .def(nb::init<double>(), "spread"_a)
        .def("spread", &graph::DummyValue4::spread);

    nb::class_<graph::DummyValue5, graph::GraphValue>(m, "DummyValue5")
        .def(nb::init<double>(), "alpha"_a)
        .def("alpha", &graph::DummyValue5::alpha);

    nb::class_<graph::DummyValuePy, graph::GraphValue>(m, "DummyValuePy")
        .def(nb::init<double>(), "value"_a)
        .def("value", &graph::DummyValuePy::value);

    // Graph Builders

    nb::class_<graph::GraphBuilder, graph::PyGraphBuilder>(m, "GraphBuilder")
        .def("key", &graph::GraphBuilder::key)
        .def("dependencies",
             [](const graph::GraphBuilder& builder) { return key_set_to_vector(builder.dependencies()); })
        .def("value", &graph::GraphBuilder::value, "graph"_a);

    nb::class_<graph::DummyGraphBuilder1, graph::GraphBuilder>(m, "DummyGraphBuilder1")
        .def(nb::init<const graph::DummyKey1&>(), "key"_a);

    nb::class_<graph::DummyGraphBuilder2, graph::GraphBuilder>(m, "DummyGraphBuilder2")
        .def(nb::init<const graph::DummyKey2&>(), "key"_a);

    nb::class_<graph::DummyGraphBuilder3, graph::GraphBuilder>(m, "DummyGraphBuilder3")
        .def(nb::init<const graph::DummyKey3&>(), "key"_a);

    nb::class_<graph::DummyGraphBuilder4, graph::GraphBuilder>(m, "DummyGraphBuilder4")
        .def(nb::init<const graph::DummyKey4&>(), "key"_a);

    nb::class_<graph::DummyGraphBuilder5, graph::GraphBuilder>(m, "DummyGraphBuilder5")
        .def(nb::init<const graph::DummyKey5&>(), "key"_a);

    m.def("make_builder", [](const graph::DummyKey1& key) { return graph::make_builder(key); }, "key"_a);
    m.def("make_builder", [](const graph::DummyKey2& key) { return graph::make_builder(key); }, "key"_a);
    m.def("make_builder", [](const graph::DummyKey3& key) { return graph::make_builder(key); }, "key"_a);
    m.def("make_builder", [](const graph::DummyKey4& key) { return graph::make_builder(key); }, "key"_a);
    m.def("make_builder", [](const graph::DummyKey5& key) { return graph::make_builder(key); }, "key"_a);
    m.def("make_builder", [](const graph::DummyKeyPy& key) { return graph::make_builder(key); }, "key"_a);

    // Graph

    nb::class_<graph::Graph>(m, "Graph")
        .def(nb::init<>())
        .def("get_value",
             static_cast<CPtr<graph::GraphValue> (graph::Graph::*)(const graph::GraphKey&) const>(
                 &graph::Graph::get_value),
             "key"_a)
        .def("insert",
             nb::overload_cast<const graph::GraphKey&, CPtr<graph::GraphValue>>(
                 &graph::Graph::insert),
             "key"_a,
             "value"_a)
        .def("insert", nb::overload_cast<const graph::GraphKey&>(&graph::Graph::insert), "key"_a)
        .def("insert",
             [](graph::Graph& graph, const std::vector<graph::GraphKey>& keys)
             {
                 graph::KeySet key_set;
                 for (const auto& key : keys)
                 {
                     key_set.insert(key);
                 }
                 graph.insert(key_set);
             },
             "keys"_a)
        .def("set_value", &graph::Graph::set_value, "key"_a, "value"_a)
        .def("compute", &graph::Graph::compute)
        .def("empty", &graph::Graph::empty)
        .def("keys", [](const graph::Graph& graph) { return key_set_to_vector(graph.keys()); })
        .def("contains",
             static_cast<bool (graph::Graph::*)(const graph::GraphKey&) const>(
                 &graph::Graph::contains),
             "key"_a);

    graph::register_py_builder_factory(
        [](const std::string& value_type_name, const graph::GraphKey& key) -> CPtr<graph::GraphBuilder>
        {
            return py_bridge::call_python_class<CPtr<graph::GraphBuilder>>(
                "graph.graph_obj_builders", value_type_name, key);
        });
}
