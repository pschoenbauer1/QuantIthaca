#pragma once

#include <nanobind/nanobind.h>

#include <stdexcept>
#include <string>
#include <utility>


namespace nb = nanobind;

namespace callback {

template <typename ReturnT, typename... Args>
inline ReturnT nb_callback(const std::string& py_function, Args&&... args) {
    try {
        nb::object mod = nb::module_::import_("cpp_callbacks");
        nb::object func = mod.attr(py_function.c_str());
        nb::object result = func(std::forward<Args>(args)...);
        return nb::cast<ReturnT>(result);
    } catch (const nb::python_error& e) {
        // e.what() includes Python traceback text
        throw std::runtime_error(std::string("Python error: ") + e.what());
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Error: ") + e.what());
    }
}

}  // namespace callback
