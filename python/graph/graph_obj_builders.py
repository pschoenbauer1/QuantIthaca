"""Python GraphBuilder implementations.

Each builder class is named Key::value_type_name() + "Builder"
(e.g. DummyValuePyBuilder for DummyKeyPy).
"""

import core_bind as cb


class DummyValuePyBuilder(cb.GraphBuilder):
    def __init__(self, key):
        super().__init__()
        self._key = key

    def key(self):
        return self._key

    def dependencies(self):
        return []

    def value(self, graph):
        return cb.DummyValuePy(self._key.x / self._key.y)


class PyValueBuilder(cb.GraphBuilder):
    def __init__(self, key):
        super().__init__()
        self._key = key

    def key(self):
        return self._key

    def dependencies(self):
        return []

    def value(self, graph):
        return StringPyValue(self._key.id)


class StringPyValue(cb.PyValue):
    """Example Python-defined value type."""

    def __init__(self, text):
        super().__init__()
        self._text = text

    def type_name(self):
        return "StringPyValue"

    def text(self):
        return self._text
