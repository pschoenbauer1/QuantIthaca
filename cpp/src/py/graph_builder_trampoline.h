#pragma once

#include <context/graph_value.h>

#include <nanobind/nanobind.h>
#include <nanobind/trampoline.h>

#include <vector>

#include <utils/exception.h>

namespace nb = nanobind;

namespace graph
{

struct PyGraphBuilder : GraphBuilder
{
    NB_TRAMPOLINE(GraphBuilder, 1);

    GraphKey key() const override
    {
        if (nb::object self = nb::find(this))
        {
            nb::gil_scoped_acquire gil;
            return nb::cast<GraphKey>(self.attr("key")());
        }
        THROW << "GraphBuilder::key() is not implemented.";
    }

    KeySet dependencies() const override
    {
        if (nb::object self = nb::find(this))
        {
            nb::gil_scoped_acquire gil;
            const auto vec = nb::cast<std::vector<GraphKey>>(self.attr("dependencies")());
            return KeySet(vec.begin(), vec.end());
        }
        THROW << "GraphBuilder::dependencies() is not implemented.";
    }

    CPtr<GraphValue> value(const Graph& graph) const override { NB_OVERRIDE(value, graph); }
};

}  // namespace graph
