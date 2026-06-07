
#include "dummy_obj1.h"

#include <context/graph_node.h>

graph::DummyGraphBuilder1::DummyGraphBuilder1(const DummyKey1& key) : _key(key)
{
    _dependencies.insert(GraphKey(DummyKey2{.str = "key1dep"}));
}
