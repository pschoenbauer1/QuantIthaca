#pragma once

#include <context_obj/py_obj.h>

#include <nanobind/nanobind.h>
#include <nanobind/trampoline.h>

namespace nb = nanobind;

namespace graph
{

struct PyGraphValue : PyValue
{
    NB_TRAMPOLINE(PyValue, 1);

    std::string type_name() const override { NB_OVERRIDE(type_name); }
};

}  // namespace graph
