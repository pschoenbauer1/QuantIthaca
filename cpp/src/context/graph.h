#pragma once

#include "exception.h"
#include "graph_key.h"
#include "graph_value.h"
#include "hash.h"
#include "pointers.h"
#include "threads.h"

namespace graph {

using KeyValueMap = std::unordered_map<GraphKey, CPtr<ValueWrapper>, utils::TupleHash>;

template <typename T>
concept KeyLike = requires(const T t) {
    typename T::ValueType;

    { T::name() } -> std::convertible_to<std::string>;
    { t.to_string() } -> std::convertible_to<std::string>;
    { T::ValueType::name() } -> std::convertible_to<std::string>;
    { t.to_tuple() };
};

class Graph {
    struct GraphImpl;
    Ptr<GraphImpl> _impl;

    void __set_value(const GraphKey& key, const CPtr<GraphValue>& value);

   public:
    Graph();
    Graph(std::unordered_map<GraphKey, CPtr<ValueWrapper>, utils::TupleHash> map);

    CPtr<GraphValue> value(const GraphKey& key) const;

    template <KeyLike Key>
    CPtr<typename Key::ValueType> value(const Key& key) const {
        const auto& v = this->value(GraphKey(key));
        auto v_casted = std::dynamic_pointer_cast<const Key::ValueType>(v);
        if (!v_casted) {
            THROW << "Error fetching " << key.to_string()
                  << ". Wrong ValueType was returned; expected" << Key::ValueType::name() << ".";
        }
        return v_casted;
    }

    template <KeyLike Key>
    void set_value(const Key& key, const CPtr<typename Key::ValueType>& value) {
        __set_value(key, value);
    }
};
}  // namespace graph