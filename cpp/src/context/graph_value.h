#pragma once

#include <context_obj/graph_key.h>
#include <utils/pointers.h>
#include <utils/threads.h>

#include <utils/exception.h>

#include <concepts>
#include <functional>
#include <string>
#include <utility>
#include <variant>

namespace graph
{

class Graph;

class GraphValue
{
public:
    virtual std::string type_name() const = 0;
    virtual ~GraphValue() = default;
};

// enum class ValueState
// {
//     initialized,
//     invalidated,
//     exception,
//     value,
//     deferred
// };

class ValueWrapper
{
    std::variant<std::monostate, CPtr<GraphValue>, std::exception_ptr> _data{};
    mutable utils::Mutex _mutex;

    bool _is_empty() const { return std::holds_alternative<std::monostate>(_data); }
    bool _has_value() const { return std::holds_alternative<CPtr<GraphValue>>(_data); }

    bool _has_error() const { return std::holds_alternative<std::exception_ptr>(_data); }

    void _rethrow_if_error() const
    {
        if (_has_error())
        {
            utils::ReadLock lock(_mutex);
            std::rethrow_exception(std::get<std::exception_ptr>(_data));
        }
    }
    void _throw_if_empty() const
    {
        if (_is_empty())
        {
            THROW << "Value is empty";
        }
    }

    void _set_error(std::exception_ptr exc)
    {
        if (!_is_empty())
        {
            THROW << "Attempted to set an error on a non-empty value wrapper.";
        }
        _data = exc;
    }
    void _set_value(CPtr<GraphValue> value)
    {
        if (!_is_empty())
        {
            THROW << "Attempted to set a value on a non-empty value wrapper.";
        }
        _data = std::move(value);
    }

public:
    // --- state checks ---
    bool is_empty() const
    {
        utils::ReadLock lock(_mutex);
        return _is_empty();
    }

    bool has_value() const
    {
        utils::ReadLock lock(_mutex);
        return _has_value();
    }

    bool has_error() const
    {
        utils::ReadLock lock(_mutex);
        return _has_error();
    }

    ValueWrapper() = default;
    ValueWrapper(const CPtr<GraphValue>& value) { set_value(value); }
    ValueWrapper(std::exception_ptr error) { set_error(error); }

    // --- accessors ---
    CPtr<GraphValue> value() const
    {
        utils::ReadLock lock(_mutex);

        rethrow_if_error();
        throw_if_empty();

        return std::get<CPtr<GraphValue>>(_data);
    }

    std::exception_ptr error() const { return std::get<std::exception_ptr>(_data); }

    void rethrow_if_error() const
    {
        utils::ReadLock lock(_mutex);
        _rethrow_if_error();
    }
    void throw_if_empty() const
    {
        utils::ReadLock lock(_mutex);
        _throw_if_empty();
    }

    void set_error(std::exception_ptr exc)
    {
        utils::WriteLock lock(_mutex);
        _set_error(exc);
    }
    void set_value(CPtr<GraphValue> value)
    {
        utils::WriteLock lock(_mutex);
        _set_value(value);
    }
};

class GraphBuilder
{
public:
    virtual ~GraphBuilder() = default;

    virtual GraphKey key() const = 0;
    virtual KeySet dependencies() const = 0;
    virtual CPtr<GraphValue> value(const Graph& graph) const
    {
        THROW << "This node does not implement a 'value' function. This is a data node.";
        return nullptr;
    }
};

template <typename Key>
CPtr<GraphBuilder> make_builder(const Key& key);

using PyBuilderFactoryFn =
    std::function<CPtr<GraphBuilder>(const std::string& value_type_name, const GraphKey& key)>;

void register_py_builder_factory(PyBuilderFactoryFn fn);
CPtr<GraphBuilder> make_py_builder(const std::string& value_type_name, const GraphKey& key);

inline std::string py_builder_class_name(const std::string& value_type_name)
{
    return value_type_name + "Builder";
}

}  // namespace graph