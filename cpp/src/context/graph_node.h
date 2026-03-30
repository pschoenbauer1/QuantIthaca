#pragma once

#include <concepts>
#include <string>
#include <variant>
#include <vector>

#include "graph.h"
#include "graph_key.h"
#include "graph_value.h"

namespace graph {

class Graph;

class Node {
   public:
    virtual ~Node() = default;

    virtual GraphKey key() const = 0;
    virtual const KeySet& dependencies() const = 0;
    virtual CPtr<GraphValue> value(const Graph& graph) const {
        THROW << "This node does not implement a 'value' function. This is a data node.";
        return nullptr;
    }
};

class DummyNode1 : public Node {
    DummyKey1 _key;
    KeySet _dependencies;

   public:
    DummyNode1(const DummyKey1&);
    GraphKey key() const override { return _key; }
    const KeySet& dependencies() const override { return _dependencies; }
    // CPtr<GraphValue> value(const Graph& graph) const override;
};

class DummyNode2 : public Node {
    DummyKey2 _key;
    KeySet _dependencies;

   public:
    DummyNode2(const DummyKey2&);
    GraphKey key() const override { return _key; }
    const KeySet& dependencies() const override { return _dependencies; }
    // CPtr<GraphValue> value(const Graph& graph) const override;
};

}  // namespace graph