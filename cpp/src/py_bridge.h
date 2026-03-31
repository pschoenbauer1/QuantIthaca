#pragma once

#include <string>
#include <utility>

#if defined(PY_BRIDGE_USE_NANOBIND)
#include "nb_callback.h"
#elif defined(PY_BRIDGE_USE_PYBIND)
#include "py_calls.h"
#else
#error "Define exactly one backend: PY_BRIDGE_USE_NANOBIND or PY_BRIDGE_USE_PYBIND"
#endif

namespace py_bridge {

template <typename ReturnT, typename... Args>
ReturnT call_python(const std::string& py_function, Args&&... args) {
#if defined(PY_BRIDGE_USE_NANOBIND)
    return callback::nb_callback<ReturnT>(py_function, std::forward<Args>(args)...);
#else
    return py_calls::py_call<ReturnT>(py_function, std::forward<Args>(args)...);
#endif
}

}  // namespace py_bridge
