"""Batch computation hooks invoked from C++ during Graph.compute()."""

import core_bind as cb


def batch_compute_leaf_python_nodes(graph) -> None:
    """Compute dependency-free Python nodes and insert their values into graph."""
    for key in graph.keys():
        if not graph.is_empty(key):
            continue
        builder = cb.make_builder(key)
        if not cb.is_python_builder(builder):
            continue
        if builder.dependencies():
            continue
        graph.set_value(key, builder.value(graph))
