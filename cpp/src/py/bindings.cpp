#include <context/graph.h>
#include <context_obj/dummy_obj1.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include <variant>

#include "core.hpp"
#include "py_bridge.h"

namespace nb = nanobind;
using namespace nb::literals;

// namespace
// {
// template <typename>
// struct Types;

// template <typename T, template <typename...> typename V, typename T, typename... Ts>
//     struct Types < std::variant<T, Ts...>
// {
//     template <typename M>
//     void add_variant(M& m)
//     {
//         m.def(T::name(), T, )
//     }
// };
// }  // namespace

NB_MODULE(core_bind, m)
{
    m.doc() = "QuantIthaca native module";
    m.def("add", &qi::add, "Add two integers");
    m.def("cpp_callback", [&](const std::string& py_func, const std::string& arg)
          { return py_bridge::call_python<std::string>(py_func, arg); });

    // Graph

    nb::class_<graph::GraphValue>(m, "GraphValue");

    nb::class_<graph::Graph>(m, "Graph")
        .def(nb::init<>())
        .def("value",
             static_cast<CPtr<graph::GraphValue> (graph::Graph::*)(const graph::GraphKey&) const>(
                 &graph::Graph::value),
             "key"_a)
        .def("insert",
             nb::overload_cast<const graph::GraphKey&, CPtr<graph::GraphValue>>(
                 &graph::Graph::insert),
             "key"_a, "value"_a)
        .def("insert", nb::overload_cast<const graph::GraphKey&>(&graph::Graph::insert), "key"_a)
        .def("insert", nb::overload_cast<const graph::KeySet&>(&graph::Graph::insert), "keys");

    // Graph Key

    nb::class_<graph::DummyKey1>(m, "DummyKey1")
        .def(nb::init<int>(), "idx"_a)
        .def_rw("idx", &graph::DummyKey1::idx);

    nb::class_<graph::DummyKey2>(m, "DummyKey2")
        .def(nb::init<std::string>(), "str"_a)
        .def_rw("idx", &graph::DummyKey2::str);

    // Graph Value

    nb::class_<graph::DummyValue1>(m, "DummyValue1")
        .def(nb::init<double>(), "x"_a)
        .def("get_x", &graph::DummyValue1::get_x);

    nb::class_<graph::DummyValue2>(m, "DummyValue2")
        .def(nb::init<double>(), "y"_a)
        .def("get_y", &graph::DummyValue2::get_y);
}
