#pragma once

#include <utils/pointers.h>

#include <cstddef>
#include <string>
#include <typeindex>

namespace graph
{

class GraphBuilder;

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

}  // namespace graph
