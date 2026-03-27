#pragma once

#pragma once

#include <memory>

#include "exception.h"

template <typename T>
using Ptr = std::shared_ptr<T>;

template <typename T>
using CPtr = std::shared_ptr<const T>;

template <typename T>
using UPtr = std::unique_ptr<T>;

template <typename T>
using CUPtr = std::unique_ptr<const T>;

template <typename T>
using UPtr = std::unique_ptr<T>;

template <typename T>
using WPtr = std::weak_ptr<T>;

template <typename T>
using CWPtr = std::weak_ptr<const T>;

namespace {
// C++ does not allow partial function template spezialiation
// Therefore we need this workaround.

template <template <typename U> typename PointerType, typename T>
struct MakePointer {};
template <typename T>
struct MakePointer<CPtr, T> {
    template <typename... Ts>
    static auto make(Ts&&... ts) {
        return std::make_shared<const T>(std::forward<Ts>(ts)...);
    }
};
template <typename T>
struct MakePointer<Ptr, T> {
    template <typename... Ts>
    static auto make(Ts&&... ts) {
        return std::make_shared<T>(std::forward<Ts>(ts)...);
    }
};
}  // namespace

template <template <typename U> typename Ptr, typename U, typename... Ts>
Ptr<U> make(Ts&&... ts) {
    return MakePointer<Ptr, U>::make(std::forward<Ts>(ts)...);
}

template <typename T, typename... Ts>
auto make_cptr(Ts&&... ts) {
    return std::make_shared<const T>(std::forward<Ts>(ts)...);
}
template <typename T, typename... Ts>
auto make_ptr(Ts&&... ts) {
    return std::make_shared<T>(std::forward<Ts>(ts)...);
}