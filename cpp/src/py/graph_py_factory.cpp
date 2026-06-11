#include "graph_py_factory.h"

#include <context/graph.h>
#include <context/graph_value.h>
#include <context_obj/dummy_obj.h>
#include <context_obj/graph_key.h>
#include <context_obj/py_obj.h>
#include <py/py_bridge.h>

#if defined(PY_BRIDGE_USE_PYBIND)
#include <pybind11/embed.h>
#include <pybind11/stl.h>

#include <vector>

namespace py = pybind11;

namespace graph
{
namespace
{

py::object graph_key_to_python(const GraphKey& key)
{
    py::module_ cb = py::module_::import("core_bind");
    return std::visit(
        [&](const auto& k) -> py::object
        {
            using Key = std::decay_t<decltype(k)>;
            if constexpr (std::is_same_v<Key, DummyKey1>)
            {
                return cb.attr("DummyKey1")(k.symbol);
            }
            else if constexpr (std::is_same_v<Key, DummyKey2>)
            {
                return cb.attr("DummyKey2")(k.index);
            }
            else if constexpr (std::is_same_v<Key, DummyKey3>)
            {
                return cb.attr("DummyKey3")(k.symbol);
            }
            else if constexpr (std::is_same_v<Key, DummyKey4>)
            {
                return cb.attr("DummyKey4")(k.symbol, k.index);
            }
            else if constexpr (std::is_same_v<Key, DummyKey5>)
            {
                return cb.attr("DummyKey5")(k.symbol, k.index);
            }
            else if constexpr (std::is_same_v<Key, DummyKeyPy>)
            {
                return cb.attr("DummyKeyPy")(k.x, k.y);
            }
            else if constexpr (std::is_same_v<Key, PyKey>)
            {
                return cb.attr("PyKey")(k.id);
            }
        },
        key);
}

GraphKey python_to_graph_key(const py::object& key)
{
    const std::string cls = py::str(key.get_type().attr("__name__")).cast<std::string>();
    if (cls == "DummyKey1")
    {
        return GraphKey(DummyKey1{.symbol = key.attr("symbol").cast<std::string>()});
    }
    if (cls == "DummyKey2")
    {
        return GraphKey(DummyKey2{.index = key.attr("index").cast<std::string>()});
    }
    if (cls == "DummyKey3")
    {
        return GraphKey(DummyKey3{.symbol = key.attr("symbol").cast<std::string>()});
    }
    if (cls == "DummyKey4")
    {
        return GraphKey(DummyKey4{.symbol = key.attr("symbol").cast<std::string>(),
                                  .index = key.attr("index").cast<std::string>()});
    }
    if (cls == "DummyKey5")
    {
        return GraphKey(DummyKey5{.symbol = key.attr("symbol").cast<std::string>(),
                                  .index = key.attr("index").cast<std::string>()});
    }
    if (cls == "DummyKeyPy")
    {
        return GraphKey(
            DummyKeyPy{.x = key.attr("x").cast<int>(), .y = key.attr("y").cast<int>()});
    }
    if (cls == "PyKey")
    {
        return GraphKey(PyKey{.id = key.attr("id").cast<std::string>()});
    }
    THROW << "Unsupported Python graph key type: " << cls;
}

class EmbeddedPyValue : public PyValue
{
    py::object _self;

public:
    explicit EmbeddedPyValue(py::object self) : _self(std::move(self)) {}

    std::string type_name() const override
    {
        py::gil_scoped_acquire gil;
        return _self.attr("type_name")().cast<std::string>();
    }
};

bool is_python_py_value(const py::object& value)
{
    py::module_ cb = py::module_::import("core_bind");
    const py::object py_value_type = cb.attr("PyValue");
    const py::object builtins = py::module_::import("builtins");
    if (builtins.attr("isinstance")(value, py_value_type).cast<bool>())
    {
        return true;
    }
    const py::object value_type = value.attr("__class__");
    return builtins.attr("issubclass")(value_type, py_value_type).cast<bool>();
}

CPtr<GraphValue> python_to_graph_value(const py::object& value)
{
    // PyValue and Python subclasses (e.g. StringPyValue) are nanobind types; hold the
    // py object and delegate — pybind11 cannot cast them to C++ PyValue directly.
    if (is_python_py_value(value))
    {
        return std::make_shared<EmbeddedPyValue>(value);
    }

    const std::string type_name = value.attr("type_name")().cast<std::string>();
    if (type_name == "DummyValue1")
    {
        return std::make_shared<DummyValue1>(value.attr("price")().cast<double>());
    }
    if (type_name == "DummyValue2")
    {
        return std::make_shared<DummyValue2>(value.attr("rate")().cast<double>());
    }
    if (type_name == "DummyValue3")
    {
        return std::make_shared<DummyValue3>(value.attr("adjusted_price")().cast<double>());
    }
    if (type_name == "DummyValue4")
    {
        return std::make_shared<DummyValue4>(value.attr("spread")().cast<double>());
    }
    if (type_name == "DummyValue5")
    {
        return std::make_shared<DummyValue5>(value.attr("alpha")().cast<double>());
    }
    if (type_name == "DummyValuePy")
    {
        return std::make_shared<DummyValuePy>(value.attr("value")().cast<double>());
    }
    THROW << "Unsupported Python graph value type: " << type_name;
}

class PyGraphBuilderEmbed : public PythonGraphBuilder
{
    py::object _self;

public:
    explicit PyGraphBuilderEmbed(py::object self) : _self(std::move(self)) {}

    GraphKey key() const override
    {
        py::gil_scoped_acquire gil;
        return python_to_graph_key(_self.attr("key")());
    }

    KeySet dependencies() const override
    {
        py::gil_scoped_acquire gil;
        py::list py_deps = _self.attr("dependencies")();
        KeySet deps;
        for (const py::handle item : py_deps)
        {
            deps.insert(python_to_graph_key(py::reinterpret_borrow<py::object>(item)));
        }
        return deps;
    }

    CPtr<GraphValue> value(const Graph& graph) const override
    {
        py::gil_scoped_acquire gil;
        // Graph is bound via nanobind in core_bind, not pybind11 in the embed exe.
        // Builders that need graph access are not supported in embedded gtests yet.
        (void)graph;
        return python_to_graph_value(_self.attr("value")(py::none()));
    }
};

CPtr<GraphBuilder> make_embedded_python_builder(const std::string& value_type_name,
                                                const GraphKey& key)
{
    py::gil_scoped_acquire gil;
    py::module_ builders = py::module_::import("graph.graph_obj_builders");
    py::object cls = builders.attr(py_builder_class_name(value_type_name).c_str());
    py::object inst = cls(graph_key_to_python(key));
    return std::make_shared<PyGraphBuilderEmbed>(std::move(inst));
}

py::object borrow_graph_for_python(Graph& graph)
{
    static py::module_::module_def mod_def;
    static py::module_ mod = []()
    {
        py::module_ m = py::module_::create_extension_module("graph._graph_embed", "", &mod_def);
        py::class_<Graph>(m, "Graph")
            .def("keys",
                 [](const Graph& g)
                 {
                     py::list out;
                     for (const auto& key : g.keys())
                     {
                         out.append(graph_key_to_python(key));
                     }
                     return out;
                 })
            .def("is_empty",
                 [](const Graph& g, const py::object& key)
                 { return g.is_empty(python_to_graph_key(key)); })
            .def("set_value",
                 [](Graph& g, const py::object& key, const py::object& value)
                 { g.set_value(python_to_graph_key(key), python_to_graph_value(value)); });
        py::module_::import("sys").attr("modules")["graph._graph_embed"] = m;
        return m;
    }();
    (void)mod;
    return py::cast(&graph, py::return_value_policy::reference);
}

}  // namespace

void install_py_builder_factory()
{
    register_py_builder_factory(make_embedded_python_builder);
}

void install_py_batch_compute()
{
    register_py_batch_compute_leaf_nodes(
        [](Graph& graph)
        {
            py_bridge::call_python_module<void>("graph.batch_compute",
                                                "batch_compute_leaf_python_nodes",
                                                borrow_graph_for_python(graph));
        });
}

}  // namespace graph

#else

#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>
#include <py/nb_callback.h>

namespace nb = nanobind;

namespace graph
{
namespace
{

CPtr<GraphBuilder> make_nanobind_python_builder(const std::string& value_type_name,
                                              const GraphKey& key)
{
    nb::object mod = nb::module_::import_("graph.graph_obj_builders");
    nb::object cls = mod.attr(py_builder_class_name(value_type_name).c_str());
    nb::object inst = cls(key);
    return nb::cast<CPtr<GraphBuilder>>(inst);
}

}  // namespace

void install_py_builder_factory()
{
    register_py_builder_factory(make_nanobind_python_builder);
}

void install_py_batch_compute()
{
    register_py_batch_compute_leaf_nodes(
        [](Graph& graph)
        {
            callback::nb_callback_module<void>(
                "graph.batch_compute",
                "batch_compute_leaf_python_nodes",
                nb::cast(&graph, nb::rv_policy::reference));
        });
}

}  // namespace graph

#endif
