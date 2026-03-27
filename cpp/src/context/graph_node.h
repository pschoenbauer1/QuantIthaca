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

    virtual std::vector<GraphKey> dependencies() const = 0;
    virtual CPtr<GraphValue> value(const Graph& graph) const = 0;
};

class DummyNode1 : public Node {
   public:
    DummyNode1(const DummyKey1&);
    std::vector<GraphKey> dependencies() const override;
    CPtr<GraphValue> value(const Graph& graph) const override;
};
class DummyNode2 : public Node {
   public:
    DummyNode2(const DummyKey2&);
    std::vector<GraphKey> dependencies() const override;
    CPtr<GraphValue> value(const Graph& graph) const override;
};

template <KeyLike Key>
CPtr<Node> get_node(const Key& key);

CPtr<Node> get_node(const DummyKey1& key) {
    return std::make_shared<DummyNode1>(key);
}
CPtr<Node> get_node(const DummyKey2& key) {
    return std::make_shared<DummyNode2>(key);
}

}  // namespace graph