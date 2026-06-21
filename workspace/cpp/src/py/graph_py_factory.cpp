#include "graph_py_factory.h"

#include <context/graph.h>
#include <context/graph_value.h>
#include <context/py_obj.h>
#include <context_obj/dummy_obj.h>
#include <py/py_bridge.h>

#if defined(PY_BRIDGE_USE_PYBIND)
#include <pybind11/embed.h>
#include <pybind11/stl.h>

#include <unordered_map>
#include <vector>

namespace py = pybind11;

namespace graph
{
namespace
{

// ---------- Pybind11 key converter registry ----------

std::unordered_map<std::string, PyToPyFn>  g_pb_to_py;   // keyed by typeid.name()
std::unordered_map<std::string, PyFromFn>  g_pb_from_py;  // keyed by Python class name

py::object graph_key_to_python_impl(const GraphKey& key)
{
    const auto it = g_pb_to_py.find(key.type_index().name());
    if (it == g_pb_to_py.end())
        THROW << "No pybind11 Python converter registered for key type: " << key.name();
    return it->second(key.impl());
}

GraphKey python_to_graph_key_impl(py::object obj)
{
    const std::string cls = py::str(obj.get_type().attr("__name__")).cast<std::string>();
    const auto it = g_pb_from_py.find(cls);
    if (it == g_pb_from_py.end())
        THROW << "No pybind11 C++ converter registered for Python key class: " << cls;
    return it->second(std::move(obj));
}

// ---------- PyValue handling ----------

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

CPtr<GraphValue> python_to_graph_value(const py::object& value)
{
    if (is_python_py_value(value))
    {
        return std::make_shared<EmbeddedPyValue>(value);
    }

    const std::string type_name = value.attr("type_name")().cast<std::string>();
    if (type_name == "DummyValue1")
        return std::make_shared<DummyValue1>(value.attr("price")().cast<double>());
    if (type_name == "DummyValue2")
        return std::make_shared<DummyValue2>(value.attr("rate")().cast<double>());
    if (type_name == "DummyValue3")
        return std::make_shared<DummyValue3>(value.attr("adjusted_price")().cast<double>());
    if (type_name == "DummyValue4")
        return std::make_shared<DummyValue4>(value.attr("spread")().cast<double>());
    if (type_name == "DummyValue5")
        return std::make_shared<DummyValue5>(value.attr("alpha")().cast<double>());
    if (type_name == "DummyValuePy")
        return std::make_shared<DummyValuePy>(value.attr("value")().cast<double>());
    THROW << "Unsupported Python graph value type: " << type_name;
}

// ---------- Python builder trampoline (pybind11 embedded path) ----------

class PyGraphBuilderEmbed : public PythonGraphBuilder
{
    py::object _self;

public:
    explicit PyGraphBuilderEmbed(py::object self) : _self(std::move(self)) {}

    GraphKey key() const override
    {
        py::gil_scoped_acquire gil;
        return python_to_graph_key_impl(_self.attr("key")());
    }

    KeySet dependencies() const override
    {
        py::gil_scoped_acquire gil;
        py::list py_deps = _self.attr("dependencies")();
        KeySet deps;
        for (const py::handle item : py_deps)
        {
            deps.insert(python_to_graph_key_impl(py::reinterpret_borrow<py::object>(item)));
        }
        return deps;
    }

    CPtr<GraphValue> value(const Graph& graph) const override
    {
        py::gil_scoped_acquire gil;
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
    py::object inst = cls(graph_key_to_python_impl(key));
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
                         out.append(graph_key_to_python_impl(key));
                     }
                     return out;
                 })
            .def("is_empty", [](const Graph& g, const py::object& key)
                 { return g.is_empty(python_to_graph_key_impl(key)); })
            .def("set_value", [](Graph& g, const py::object& key, const py::object& value)
                 { g.set_value(python_to_graph_key_impl(key), python_to_graph_value(value)); });
        py::module_::import("sys").attr("modules")["graph._graph_embed"] = m;
        return m;
    }();
    (void)mod;
    return py::cast(&graph, py::return_value_policy::reference);
}

}  // namespace

void register_pybind_key_converter(const char* type_id_name,
                                   const char* py_class_name,
                                   PyToPyFn   to_py,
                                   PyFromFn   from_py)
{
    g_pb_to_py[type_id_name]   = std::move(to_py);
    g_pb_from_py[py_class_name] = std::move(from_py);
}

py::object graph_key_to_python_pb(const GraphKey& key)
{
    return graph_key_to_python_impl(key);
}

GraphKey python_to_graph_key_pb(py::object obj)
{
    return python_to_graph_key_impl(std::move(obj));
}

void install_py_builder_factory()
{
    // Register key converters for all built-in key types (pybind11 embedded path).
    auto reg = [](const char* tid, const char* cls, PyToPyFn to, PyFromFn from)
    {
        register_pybind_key_converter(tid, cls, std::move(to), std::move(from));
    };

    reg(typeid(DummyKey1).name(), "DummyKey1",
        [](const IGraphKey& k) {
            py::module_ cb = py::module_::import("core_bind");
            return cb.attr("DummyKey1")(static_cast<const KeyModel<DummyKey1>&>(k).get().symbol);
        },
        [](py::object o) -> GraphKey {
            return DummyKey1{.symbol = o.attr("symbol").cast<std::string>()};
        });

    reg(typeid(DummyKey2).name(), "DummyKey2",
        [](const IGraphKey& k) {
            py::module_ cb = py::module_::import("core_bind");
            return cb.attr("DummyKey2")(static_cast<const KeyModel<DummyKey2>&>(k).get().index);
        },
        [](py::object o) -> GraphKey {
            return DummyKey2{.index = o.attr("index").cast<std::string>()};
        });

    reg(typeid(DummyKey3).name(), "DummyKey3",
        [](const IGraphKey& k) {
            py::module_ cb = py::module_::import("core_bind");
            return cb.attr("DummyKey3")(static_cast<const KeyModel<DummyKey3>&>(k).get().symbol);
        },
        [](py::object o) -> GraphKey {
            return DummyKey3{.symbol = o.attr("symbol").cast<std::string>()};
        });

    reg(typeid(DummyKey4).name(), "DummyKey4",
        [](const IGraphKey& k) {
            py::module_ cb = py::module_::import("core_bind");
            const auto& key = static_cast<const KeyModel<DummyKey4>&>(k).get();
            return cb.attr("DummyKey4")(key.symbol, key.index);
        },
        [](py::object o) -> GraphKey {
            return DummyKey4{.symbol = o.attr("symbol").cast<std::string>(),
                             .index  = o.attr("index").cast<std::string>()};
        });

    reg(typeid(DummyKey5).name(), "DummyKey5",
        [](const IGraphKey& k) {
            py::module_ cb = py::module_::import("core_bind");
            const auto& key = static_cast<const KeyModel<DummyKey5>&>(k).get();
            return cb.attr("DummyKey5")(key.symbol, key.index);
        },
        [](py::object o) -> GraphKey {
            return DummyKey5{.symbol = o.attr("symbol").cast<std::string>(),
                             .index  = o.attr("index").cast<std::string>()};
        });

    reg(typeid(DummyKeyPy).name(), "DummyKeyPy",
        [](const IGraphKey& k) {
            py::module_ cb = py::module_::import("core_bind");
            const auto& key = static_cast<const KeyModel<DummyKeyPy>&>(k).get();
            return cb.attr("DummyKeyPy")(key.x, key.y);
        },
        [](py::object o) -> GraphKey {
            return DummyKeyPy{.x = o.attr("x").cast<int>(), .y = o.attr("y").cast<int>()};
        });

    reg(typeid(PyKey).name(), "PyKey",
        [](const IGraphKey& k) {
            py::module_ cb = py::module_::import("core_bind");
            return cb.attr("PyKey")(static_cast<const KeyModel<PyKey>&>(k).get().id);
        },
        [](py::object o) -> GraphKey {
            return PyKey{.id = o.attr("id").cast<std::string>()};
        });

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
#include <py/graph_key_caster.h>
#include <py/nb_callback.h>

#include <unordered_map>

namespace nb = nanobind;

namespace graph
{
namespace
{

// ---------- Nanobind key converter registry ----------

std::unordered_map<std::string, NbToPyFn>  g_nb_to_py;    // keyed by typeid.name()
std::unordered_map<std::string, NbFromFn>  g_nb_from_py;   // keyed by Python class name

bool is_nanobind_py_value(const nb::handle& value)
{
    const nb::object py_value_type = nb::module_::import_("core_bind").attr("PyValue");
    return nb::cast<bool>(
        nb::module_::import_("builtins").attr("isinstance")(value, py_value_type));
}

CPtr<GraphBuilder> make_nanobind_python_builder(const std::string& value_type_name,
                                                const GraphKey& key)
{
    nb::object mod  = nb::module_::import_("graph.graph_obj_builders");
    nb::object cls  = mod.attr(py_builder_class_name(value_type_name).c_str());
    nb::object inst = cls(graph_key_to_python(key));
    return nb::cast<CPtr<GraphBuilder>>(inst);
}

}  // namespace

void register_nb_key_converter(const char* type_id_name,
                               const char* py_class_name,
                               NbToPyFn   to_py,
                               NbFromFn   from_py)
{
    g_nb_to_py[type_id_name]    = std::move(to_py);
    g_nb_from_py[py_class_name] = std::move(from_py);
}

nb::object graph_key_to_python(const GraphKey& key)
{
    const auto it = g_nb_to_py.find(key.type_index().name());
    if (it == g_nb_to_py.end())
        THROW << "No nanobind Python converter registered for key type: " << key.name();
    return it->second(key.impl());
}

GraphKey python_to_graph_key(nb::handle h)
{
    const std::string cls =
        nb::cast<std::string>(h.type().attr("__name__"));
    const auto it = g_nb_from_py.find(cls);
    if (it == g_nb_from_py.end())
        THROW << "No nanobind C++ converter registered for Python key class: " << cls;
    return it->second(h);
}

CPtr<GraphValue> nanobind_python_to_graph_value(nb::handle value)
{
    if (is_nanobind_py_value(value))
    {
        return nb::cast<CPtr<PyValue>>(value);
    }
    return nb::cast<CPtr<GraphValue>>(value);
}

void install_py_builder_factory()
{
    register_py_builder_factory(make_nanobind_python_builder);
}

void install_py_batch_compute()
{
    register_py_batch_compute_leaf_nodes(
        [](Graph& graph)
        {
            callback::nb_callback_module<void>("graph.batch_compute",
                                               "batch_compute_leaf_python_nodes",
                                               nb::cast(&graph, nb::rv_policy::reference));
        });
}

}  // namespace graph

#endif
