#pragma once

#include <type_traits>

namespace app {
template <typename T> struct is_bitmask_enum : std::false_type {};

template <typename E> std::enable_if_t<app::is_bitmask_enum<E>::value, bool> constexpr Any(E e) {
    return static_cast<std::underlying_type_t<E>>(e) != 0;
}
}

template <typename E>
std::enable_if_t<app::is_bitmask_enum<E>::value, E> constexpr operator|(E l, E r) {
    using U = std::underlying_type_t<E>;
    return static_cast<E>(static_cast<U>(l) | static_cast<U>(r));
}

template <typename E>
std::enable_if_t<app::is_bitmask_enum<E>::value, E&> constexpr operator|=(E& l, E r) {
    return l = l | r;
}

template <typename E>
std::enable_if_t<app::is_bitmask_enum<E>::value, E> constexpr operator&(E l, E r) {
    using U = std::underlying_type_t<E>;
    return static_cast<E>(static_cast<U>(l) & static_cast<U>(r));
}

template <typename E>
std::enable_if_t<app::is_bitmask_enum<E>::value, E&> constexpr operator&=(E& l, E r) {
    return l = l & r;
}

template <typename E>
std::enable_if_t<app::is_bitmask_enum<E>::value, E> constexpr operator^(E l, E r) {
    using U = std::underlying_type_t<E>;
    return static_cast<E>(static_cast<U>(l) ^ static_cast<U>(r));
}

template <typename E>
std::enable_if_t<app::is_bitmask_enum<E>::value, E&> constexpr operator^=(E& l, E r) {
    return l = l ^ r;
}

template <typename E>
std::enable_if_t<app::is_bitmask_enum<E>::value, E> constexpr operator~(E e) {
    return static_cast<E>(~static_cast<std::underlying_type_t<E>>(e));
}
