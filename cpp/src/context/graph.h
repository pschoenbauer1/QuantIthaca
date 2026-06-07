#pragma once

#include <context/graph_key.h>
#include <context/graph_value.h>
#include <utils/exception.h>
#include <utils/hash.h>
#include <utils/pointers.h>
#include <utils/threads.h>

#include <unordered_set>

namespace graph
{

template <typename T>
using KeyMap = std::unordered_map<GraphKey, T, utils::TupleHash>;
using KeySet = std::unordered_set<GraphKey, utils::TupleHash>;

template <typename T>
concept KeyLike = requires(const T t) {
    typename T::ValueType;

    { T::name() } -> std::convertible_to<std::string>;
    { t.to_string() } -> std::convertible_to<std::string>;
    { T::ValueType::name() } -> std::convertible_to<std::string>;
    { t.to_tuple() };
};

class Graph
{
    struct GraphImpl;
    Ptr<GraphImpl> _impl;

public:
    Graph();
    Graph(std::unordered_map<GraphKey, Ptr<ValueWrapper>, utils::TupleHash> map);
    Graph(Graph&&) = delete;
    Graph(const Graph&) = delete;
    Graph& operator=(const Graph&) = delete;
    Graph& operator=(Graph&&) = delete;

    CPtr<GraphValue> value(const GraphKey& key) const;

    template <KeyLike Key>
    CPtr<typename Key::ValueType> value(const Key& key) const
    {
        const auto& v = this->value(GraphKey(key));
        auto v_casted = std::dynamic_pointer_cast<const Key::ValueType>(v);
        if (!v_casted)
        {
            THROW << "Error fetching " << key.to_string()
                  << ". Wrong ValueType was returned; expected" << Key::ValueType::name() << ".";
        }
        return v_casted;
    }

    void insert(const GraphKey& key, CPtr<GraphValue> value);
    void insert(const GraphKey& key);
    void insert(const KeySet& keys);
};
}  // namespace graph