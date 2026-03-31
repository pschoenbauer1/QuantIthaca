#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

#include "core.hpp"
#include "py_bridge.h"

namespace nb = nanobind;

NB_MODULE(core_bind, m) {
    m.doc() = "QuantIthaca native module";
    m.def("add", &qi::add, "Add two integers");
    m.def("cpp_callback", [&](const std::string& py_func, const std::string& arg) {
        return py_bridge::call_python<std::string>(py_func, arg);
    });
}
