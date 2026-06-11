#pragma once

#include <context/graph_value.h>

#include <nanobind/nanobind.h>
#include <nanobind/trampoline.h>

#include <vector>

namespace nb = nanobind;

namespace graph
{

struct PyGraphBuilder : PythonGraphBuilder
{
    NB_TRAMPOLINE(PythonGraphBuilder, 3);

    GraphKey key() const override { NB_OVERRIDE_PURE(key); }

    KeySet dependencies() const override
    {
        nanobind::detail::ticket nb_ticket(nb_trampoline, "dependencies", true);
        const nb::object deps = nb_trampoline.base().attr(nb_ticket.key)();
        KeySet result;
        for (nb::handle item : deps)
        {
            result.insert(nb::cast<GraphKey>(item));
        }
        return result;
    }

    CPtr<GraphValue> value(const Graph& graph) const override
    {
        nanobind::detail::ticket nb_ticket(nb_trampoline, "value", false);
        if (nb_ticket.key.is_valid())
        {
            nb::object py_graph = nb::cast(&graph, nb::rv_policy::reference_internal);
            return nb::cast<CPtr<GraphValue>>(
                nb_trampoline.base().attr(nb_ticket.key)(py_graph));
        }
        return NBBase::value(graph);
    }
};

}  // namespace graph
