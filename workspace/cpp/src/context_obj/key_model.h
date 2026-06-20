#pragma once

#include <context_obj/i_graph_key.h>
#include <utils/hash.h>
#include <utils/pointers.h>

#include <functional>
#include <memory>
#include <typeindex>

namespace graph
{

// Primary template — must be specialized for each key type.
// Specializations provide: using BuilderType = ...; using ValueType = ...;
template <typename Key>
struct Mapping;

// Adapts any KeyLike struct K to the IGraphKey interface.
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

}  // namespace graph
