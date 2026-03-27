#pragma once

#include <concepts>
#include <cstddef>
#include <functional>
#include <tuple>
#include <utility>

namespace utils {
inline void hash_combine(std::size_t& seed, std::size_t value) noexcept {
    seed ^= value + 0x9e3779b9u + (seed << 6) + (seed >> 2);
}

template <class T>
inline void hash_value(std::size_t& seed, const T& value) noexcept {
    hash_combine(seed, std::hash<T>{}(value));
}

template <class T>
concept HasToTuple = requires(const T& t) { t.to_tuple(); };

struct TupleHash {
    template <HasToTuple T>
    std::size_t operator()(const T& value) const noexcept(noexcept(value.to_tuple())) {
        return std::apply(
            [](const auto&... xs) {
                std::size_t seed = 0;
                (hash_value(seed, xs), ...);
                return seed;
            },
            value.to_tuple());
    }

    template <class... Ts>
        requires(requires(const Ts& x) { HasToTuple<Ts>; } && ...)

    std::size_t operator()(const std::variant<Ts...>& v) const {
        std::size_t seed = 0;

        hash_combine(seed, v.index());

        std::visit(
            [&](const auto& x) {
                using T = std::decay_t<decltype(x)>;
                hash_combine(seed, TupleHash{}(x));
            },
            v);

        return seed;
    }
};

}  // namespace utils
