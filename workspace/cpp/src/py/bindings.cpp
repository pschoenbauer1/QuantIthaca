#include <context/graph.h>
#include <context_obj/dummy_obj.h>
#include <context_obj/graph_key.h>
#include <context_obj/key_model.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <py/graph_key_caster.h>
#include <py/graph_py_factory.h>

#include <typeindex>
#include <vector>

#include "core.hpp"
#include "graph_builder_trampoline.h"
#include "graph_py_factory.h"
#include "graph_value_trampoline.h"
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

    // Graph Keys — each binding is followed by its converter registration so they stay co-located.

    nb::class_<graph::DummyKey1>(m, "DummyKey1")
        .def(nb::init<>())
        .def(nb::init<std::string>(), "symbol"_a)
        .def_rw("symbol", &graph::DummyKey1::symbol)
        .def("__repr__", [](const graph::DummyKey1& key) { return key.to_string(); });
    graph::register_nb_key_converter(
        typeid(graph::DummyKey1).name(), "DummyKey1",
        [](const graph::IGraphKey& k) -> nb::object {
            const auto& key = static_cast<const graph::KeyModel<graph::DummyKey1>&>(k).get();
            return nb::cast(graph::DummyKey1{.symbol = key.symbol});
        },
        [](nb::handle h) -> graph::GraphKey {
            return graph::DummyKey1{.symbol = nb::cast<std::string>(h.attr("symbol"))};
        });

    nb::class_<graph::DummyKey2>(m, "DummyKey2")
        .def(nb::init<>())
        .def(nb::init<std::string>(), "index"_a)
        .def_rw("index", &graph::DummyKey2::index)
        .def("__repr__", [](const graph::DummyKey2& key) { return key.to_string(); });
    graph::register_nb_key_converter(
        typeid(graph::DummyKey2).name(), "DummyKey2",
        [](const graph::IGraphKey& k) -> nb::object {
            const auto& key = static_cast<const graph::KeyModel<graph::DummyKey2>&>(k).get();
            return nb::cast(graph::DummyKey2{.index = key.index});
        },
        [](nb::handle h) -> graph::GraphKey {
            return graph::DummyKey2{.index = nb::cast<std::string>(h.attr("index"))};
        });

    nb::class_<graph::DummyKey3>(m, "DummyKey3")
        .def(nb::init<>())
        .def(nb::init<std::string>(), "symbol"_a)
        .def_rw("symbol", &graph::DummyKey3::symbol)
        .def("__repr__", [](const graph::DummyKey3& key) { return key.to_string(); });
    graph::register_nb_key_converter(
        typeid(graph::DummyKey3).name(), "DummyKey3",
        [](const graph::IGraphKey& k) -> nb::object {
            const auto& key = static_cast<const graph::KeyModel<graph::DummyKey3>&>(k).get();
            return nb::cast(graph::DummyKey3{.symbol = key.symbol});
        },
        [](nb::handle h) -> graph::GraphKey {
            return graph::DummyKey3{.symbol = nb::cast<std::string>(h.attr("symbol"))};
        });

    nb::class_<graph::DummyKey4>(m, "DummyKey4")
        .def(nb::init<>())
        .def(nb::init<std::string, std::string>(), "symbol"_a, "index"_a)
        .def_rw("symbol", &graph::DummyKey4::symbol)
        .def_rw("index", &graph::DummyKey4::index)
        .def("__repr__", [](const graph::DummyKey4& key) { return key.to_string(); });
    graph::register_nb_key_converter(
        typeid(graph::DummyKey4).name(), "DummyKey4",
        [](const graph::IGraphKey& k) -> nb::object {
            const auto& key = static_cast<const graph::KeyModel<graph::DummyKey4>&>(k).get();
            return nb::cast(graph::DummyKey4{.symbol = key.symbol, .index = key.index});
        },
        [](nb::handle h) -> graph::GraphKey {
            return graph::DummyKey4{.symbol = nb::cast<std::string>(h.attr("symbol")),
                                    .index  = nb::cast<std::string>(h.attr("index"))};
        });

    nb::class_<graph::DummyKey5>(m, "DummyKey5")
        .def(nb::init<>())
        .def(nb::init<std::string, std::string>(), "symbol"_a, "index"_a)
        .def_rw("symbol", &graph::DummyKey5::symbol)
        .def_rw("index", &graph::DummyKey5::index)
        .def("__repr__", [](const graph::DummyKey5& key) { return key.to_string(); });
    graph::register_nb_key_converter(
        typeid(graph::DummyKey5).name(), "DummyKey5",
        [](const graph::IGraphKey& k) -> nb::object {
            const auto& key = static_cast<const graph::KeyModel<graph::DummyKey5>&>(k).get();
            return nb::cast(graph::DummyKey5{.symbol = key.symbol, .index = key.index});
        },
        [](nb::handle h) -> graph::GraphKey {
            return graph::DummyKey5{.symbol = nb::cast<std::string>(h.attr("symbol")),
                                    .index  = nb::cast<std::string>(h.attr("index"))};
        });

    nb::class_<graph::DummyKeyPy>(m, "DummyKeyPy")
        .def(nb::init<>())
        .def(nb::init<int, int>(), "x"_a, "y"_a)
        .def_rw("x", &graph::DummyKeyPy::x)
        .def_rw("y", &graph::DummyKeyPy::y)
        .def("__repr__", [](const graph::DummyKeyPy& key) { return key.to_string(); });
    graph::register_nb_key_converter(
        typeid(graph::DummyKeyPy).name(), "DummyKeyPy",
        [](const graph::IGraphKey& k) -> nb::object {
            const auto& key = static_cast<const graph::KeyModel<graph::DummyKeyPy>&>(k).get();
            return nb::cast(graph::DummyKeyPy{.x = key.x, .y = key.y});
        },
        [](nb::handle h) -> graph::GraphKey {
            return graph::DummyKeyPy{.x = nb::cast<int>(h.attr("x")),
                                     .y = nb::cast<int>(h.attr("y"))};
        });

    nb::class_<graph::PyKey>(m, "PyKey")
        .def(nb::init<>())
        .def(nb::init<std::string>(), "id"_a)
        .def_rw("id", &graph::PyKey::id)
        .def("__repr__", [](const graph::PyKey& key) { return key.to_string(); });
    graph::register_nb_key_converter(
        typeid(graph::PyKey).name(), "PyKey",
        [](const graph::IGraphKey& k) -> nb::object {
            const auto& key = static_cast<const graph::KeyModel<graph::PyKey>&>(k).get();
            return nb::cast(graph::PyKey{.id = key.id});
        },
        [](nb::handle h) -> graph::GraphKey {
            return graph::PyKey{.id = nb::cast<std::string>(h.attr("id"))};
        });

    // Graph Values

    nb::class_<graph::GraphValue>(m, "GraphValue").def("type_name", &graph::GraphValue::type_name);

    nb::class_<graph::PyValue, graph::PyGraphValue, graph::GraphValue>(m, "PyValue")
        .def(nb::init<>())
        .def("type_name", &graph::PyValue::type_name);

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

    nb::class_<graph::GraphBuilder>(m, "GraphBuilder")
        .def("key", &graph::GraphBuilder::key)
        .def("dependencies", [](const graph::GraphBuilder& builder)
             { return key_set_to_vector(builder.dependencies()); })
        .def("value", &graph::GraphBuilder::value, "graph"_a);

    nb::class_<graph::PythonGraphBuilder, graph::PyGraphBuilder, graph::GraphBuilder>(
        m, "PythonGraphBuilder")
        .def(nb::init<>())
        .def("key", &graph::PythonGraphBuilder::key)
        .def("dependencies", [](const graph::PythonGraphBuilder& builder)
             { return key_set_to_vector(builder.dependencies()); })
        .def("value", &graph::PythonGraphBuilder::value, "graph"_a);

    m.def("is_python_builder", &graph::is_python_builder, "builder"_a);

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

    m.def(
        "make_builder",
        [](const graph::GraphKey& key) { return key.make_builder(); },
        "key"_a);

    // Graph

    nb::class_<graph::Graph>(m, "Graph")
        .def("__init__", [](graph::Graph* self) { new (self) graph::Graph(); })
        .def("get_value",
             static_cast<CPtr<graph::GraphValue> (graph::Graph::*)(const graph::GraphKey&) const>(
                 &graph::Graph::get_value),
             "key"_a)
        .def("insert", nb::overload_cast<const graph::GraphKey&>(&graph::Graph::insert), "key"_a)
        .def(
            "insert",
            [](graph::Graph& graph, const graph::GraphKey& key, const nb::object& value)
            { graph.insert(key, graph::nanobind_python_to_graph_value(value)); },
            "key"_a,
            "value"_a)
        .def(
            "insert",
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
        .def(
            "set_value",
            [](graph::Graph& graph, const graph::GraphKey& key, const nb::object& value)
            { graph.set_value(key, graph::nanobind_python_to_graph_value(value)); },
            "key"_a,
            "value"_a)
        .def("compute", &graph::Graph::compute)
        .def("empty", &graph::Graph::empty)
        .def("keys", [](const graph::Graph& graph) { return key_set_to_vector(graph.keys()); })
        .def("contains",
             static_cast<bool (graph::Graph::*)(const graph::GraphKey&) const>(
                 &graph::Graph::contains),
             "key"_a)
        .def("is_empty",
             static_cast<bool (graph::Graph::*)(const graph::GraphKey&) const>(
                 &graph::Graph::is_empty),
             "key"_a);

    graph::install_py_builder_factory();
    graph::install_py_batch_compute();
}
