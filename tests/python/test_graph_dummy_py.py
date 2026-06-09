import quantithaca  # noqa: F401 — ensures core_bind is on sys.path

import core_bind as cb


def test_dummy_py_node_uses_python_builder():
    graph = cb.Graph()
    key = cb.DummyKeyPy(10, 4)

    graph.insert(key)
    graph.compute()

    assert graph.contains(key)
    assert graph.get_value(key).value() == 2.5
