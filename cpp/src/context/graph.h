#pragma once

#include <context/graph_value.h>
// #include <context_obj/dummy_obj1.h>
// #include <context_obj/dummy_obj2.h>
#include <context_obj/graph_key.h>
#include <utils/exception.h>
#include <utils/hash.h>
#include <utils/pointers.h>
#include <utils/threads.h>

#include <unordered_set>

namespace graph
{

template <typename T>
concept KeyLike = requires(const T t) {
    typename T::ValueType;

    { T::name() } -> std::convertible_to<std::string>;
    { t.to_string() } -> std::convertible_to<std::string>;
    { T::value_type_name() } -> std::convertible_to<std::string>;
    { t.to_tuple() };
};

class Graph
{
    struct GraphImpl;
    Ptr<GraphImpl> _impl;

public:
    Graph();
    Graph(Graph&&) = delete;
    Graph(const Graph&) = delete;
    Graph& operator=(const Graph&) = delete;
    Graph& operator=(Graph&&) = delete;

    CPtr<GraphValue> get_value(const GraphKey& key) const;

    template <KeyLike Key>
    CPtr<typename Key::ValueType> get_value(const Key& key) const
    {
        const auto& v = this->get_value(GraphKey(key));
        auto v_casted = std::dynamic_pointer_cast<const Key::ValueType>(v);
        if (!v_casted)
        {
            THROW << "Error fetching " << key.to_string()
                  << ". Wrong ValueType was returned; expected " << Key::value_type_name() << ".";
        }
        return v_casted;
    }

    void insert(const GraphKey& key, CPtr<GraphValue> value);
    void insert(const GraphKey& key);
    void insert(const KeySet& keys);
    
    void set_value(const GraphKey& key, CPtr<GraphValue> value);

    void compute();

    bool empty() const;
    KeySet keys() const;
    bool contains(const GraphKey& key) const;
    bool is_empty(const GraphKey& key) const;

    template <KeyLike Key>
    bool contains(const Key& key) const
    {
        return contains(GraphKey(key));
    }
};
}  // namespace graph