#pragma once

#include <concepts>
#include <string>
#include <variant>

#include "graph_value.h"

namespace graph {

struct DummyKey1 {
    int idx = 1;

    using ValueType = DummyValue1;
    static std::string name() { return "DummyKey1"; }
    std::string to_string() const { return "x"; }
    auto to_tuple() const { return std::make_tuple(idx); }
    auto operator<=>(const DummyKey1&) const = default;
};

struct DummyKey2 {
    std::string str = "hello world";

    using ValueType = DummyValue2;
    static std::string name() { return "DummyKey2"; }
    std::string to_string() const { return str; }
    auto to_tuple() const { return std::make_tuple(str); }
    auto operator<=>(const DummyKey2&) const = default;
};

using GraphKey = std::variant<DummyKey1, DummyKey2>;

inline std::string to_string(const GraphKey& key) {
    return std::visit([](const auto& key_) { return key_.to_string(); }, key);
}

}  // namespace graph