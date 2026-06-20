import quantithaca  # noqa: F401 — ensures core_bind is on sys.path

import core_bind as cb

class CustomPyValue(cb.PyValue):
    def __init__(self, payload):
        super().__init__()
        self._payload = payload

    def type_name(self):
        return "CustomPyValue"

    def payload(self):
        return self._payload


def test_py_key_computes_python_subclass_value():
    graph = cb.Graph()
    key = cb.PyKey("hello")

    graph.insert(key)
    graph.compute()

    assert graph.contains(key)
    value = graph.get_value(key)
    assert isinstance(value, cb.PyValue)
    assert value.type_name() == "StringPyValue"
    assert value.text() == "hello"


def test_py_key_accepts_python_subclass_on_insert():
    print(cb)
    graph = cb.Graph()
    key = cb.PyKey("leaf")

    graph.insert(key, CustomPyValue(42))
    graph.compute()

    assert graph.get_value(key).payload() == 42
