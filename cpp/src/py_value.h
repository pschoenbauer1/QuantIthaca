#pragma once

#include <any>
#include <variant>

#include "threads.h"

namespace py_calls {

class PyValue {
    std::variant<std::any, std::exception_ptr> _data{};
    mutable utils::Mutex _mutex;
};
}  // namespace py_calls