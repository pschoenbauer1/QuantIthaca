#pragma once

#include <utils/hash.h>
#include <utils/pointers.h>

#include <concepts>
#include <cstddef>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>

namespace graph
{

class GraphBuilder;

// ---------- IGraphKey ----------

class IGraphKey
{
public:
    virtual ~IGraphKey() = default;

    virtual std::string name() const = 0;
    virtual std::string to_string() const = 0;
    virtual std::string value_type_name() const = 0;
    virtual bool allows_py_value_subclasses() const { return false; }
    virtual std::type_index type_index() const = 0;
    virtual std::size_t hash() const = 0;
    virtual bool equals(const IGraphKey& other) const = 0;
    virtual CPtr<GraphBuilder> make_builder() const = 0;
};

// ---------- Mapping ----------

// Primary template — must be specialized for each key type.
// Each specialization provides: using BuilderType = ...; using ValueType = ...;
template <typename Key>
struct Mapping;

// ---------- KeyModel ----------

template <typename K>
class KeyModel final : public IGraphKey
{
    K _key;

public:
    explicit KeyModel(K k) : _key(std::move(k)) {}

    const K& get() const { return _key; }

    std::string name() const override { return K::name(); }
    std::string to_string() const override { return _key.to_string(); }
    std::string value_type_name() const override { return Mapping<K>::ValueType::name(); }
    bool allows_py_value_subclasses() const override { return false; }

    std::type_index type_index() const override { return typeid(K); }

    std::size_t hash() const override
    {
        std::size_t seed = std::hash<std::type_index>{}(typeid(K));
        utils::hash_combine(seed, utils::TupleHash{}(_key));
        return seed;
    }

    bool equals(const IGraphKey& other) const override
    {
        const auto* o = dynamic_cast<const KeyModel<K>*>(&other);
        return o && _key == o->_key;
    }

    CPtr<GraphBuilder> make_builder() const override
    {
        return std::make_shared<typename Mapping<K>::BuilderType>(_key);
    }
};

// ---------- KeyLike ----------

template <typename T>
concept KeyLike = requires(const T t) {
    { T::name() } -> std::convertible_to<std::string>;
    { t.to_string() } -> std::convertible_to<std::string>;
    { t.to_tuple() };
};

// ---------- GraphKey ----------

class GraphKey
{
    std::shared_ptr<const IGraphKey> _impl;

public:
    template <typename K>
        requires KeyLike<K>
    explicit(false) GraphKey(K k) : _impl(std::make_shared<KeyModel<K>>(std::move(k)))
    {}

    std::string name() const { return _impl->name(); }
    std::string to_string() const { return _impl->to_string(); }
    std::string value_type_name() const { return _impl->value_type_name(); }
    bool allows_py_value_subclasses() const { return _impl->allows_py_value_subclasses(); }
    std::type_index type_index() const { return _impl->type_index(); }
    CPtr<GraphBuilder> make_builder() const { return _impl->make_builder(); }
    const IGraphKey& impl() const { return *_impl; }

    bool operator==(const GraphKey& other) const { return _impl->equals(*other._impl); }
};

template <typename T>
using KeyMap = std::unordered_map<GraphKey, T>;
using KeySet = std::unordered_set<GraphKey>;

inline std::string to_string(const GraphKey& key)
{
    return key.to_string();
}

}  // namespace graph

template <>
struct std::hash<graph::GraphKey>
{
    std::size_t operator()(const graph::GraphKey& k) const noexcept
    {
        return k.impl().hash();
    }
};
