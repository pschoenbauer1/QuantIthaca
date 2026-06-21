#pragma once

#include <context/graph_key.h>

#include <nanobind/nanobind.h>

#include <optional>

namespace nb = nanobind;

// Forward declarations — defined in graph_py_factory.cpp (nanobind path).
namespace graph
{
nb::object graph_key_to_python(const GraphKey& key);
GraphKey python_to_graph_key(nb::handle h);
}  // namespace graph

namespace nanobind::detail
{

// GraphKey is not default-constructible, so we can't use NB_TYPE_CASTER directly.
// Store the converted value in an optional instead — mirrors the macro's interface exactly.
template <>
struct type_caster<graph::GraphKey>
{
    using Value = graph::GraphKey;
    static constexpr auto Name = const_name("GraphKey");

    template <typename T_> using Cast = movable_cast_t<T_>;
    template <typename T_> static constexpr bool can_cast() { return true; }

    template <typename T_,
              enable_if_t<std::is_same_v<std::remove_cv_t<T_>, Value>> = 0>
    static handle from_cpp(T_* p, rv_policy policy, cleanup_list* list)
    {
        if (!p) return none().release();
        return from_cpp(*p, policy, list);
    }

    explicit operator Value*() { return _value ? &*_value : nullptr; }
    explicit operator Value&() { return (Value&) *_value; }
    explicit operator Value&&() { return (Value&&) *_value; }

    bool from_python(handle src, uint8_t /*flags*/, cleanup_list* /*cleanup*/) noexcept
    {
        try
        {
            _value.emplace(graph::python_to_graph_key(src));
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    static handle from_cpp(const graph::GraphKey& key,
                           rv_policy /*policy*/,
                           cleanup_list* /*cleanup*/)
    {
        return graph::graph_key_to_python(key).release();
    }

private:
    std::optional<graph::GraphKey> _value;
};

}  // namespace nanobind::detail
