#pragma once

#include "py_runtime.h"

#include <stdexcept>
#include <string>
#include <utility>

namespace py = pybind11;

namespace py_calls
{

template <typename ReturnT, typename... Args>
ReturnT py_invoke(const std::string& py_module, const std::string& attr, Args&&... args)
{
    py_runtime::ensure_python_runtime();

    py::gil_scoped_acquire gil{};
    py::module_ mod = py::module_::import(py_module.c_str());
    py::object callable = mod.attr(attr.c_str());
    py::object result = callable(std::forward<Args>(args)...);
    return result.cast<ReturnT>();
}

template <typename ReturnT, typename... Args>
inline ReturnT py_call(const std::string& py_function, Args&&... args)
{
    try
    {
        return py_invoke<ReturnT>("cpp_callbacks", py_function, std::forward<Args>(args)...);
    }
    catch (const py::error_already_set& e)
    {
        throw std::runtime_error(std::string("Python error: ") + e.what());
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::string("Error: ") + e.what());
    }
}

template <typename ReturnT, typename... Args>
inline ReturnT py_call_module(const std::string& py_module,
                              const std::string& py_function,
                              Args&&... args)
{
    try
    {
        return py_invoke<ReturnT>(py_module, py_function, std::forward<Args>(args)...);
    }
    catch (const py::error_already_set& e)
    {
        throw std::runtime_error(std::string("Python error: ") + e.what());
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::string("Error: ") + e.what());
    }
}

template <typename ReturnT, typename... Args>
inline ReturnT py_call_class(const std::string& py_module,
                             const std::string& class_name,
                             Args&&... args)
{
    try
    {
        return py_invoke<ReturnT>(py_module, class_name, std::forward<Args>(args)...);
    }
    catch (const py::error_already_set& e)
    {
        throw std::runtime_error(std::string("Python error: ") + e.what());
    }
    catch (const std::exception& e)
    {
        throw std::runtime_error(std::string("Error: ") + e.what());
    }
}

}  // namespace py_calls
