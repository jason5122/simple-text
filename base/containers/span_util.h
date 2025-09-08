#pragma once

#include <span>
#include <type_traits>

// These are `uint8_t` variants of `std::as_writeable_bytes`/`std::as_bytes`.

namespace base {

// A tiny concept that says "std::span{t} is well-formed".
template <class T>
concept SpanConstructibleFrom = requires(T&& t) { std::span(std::forward<T>(t)); };

// Writabe: anything span-constructible
template <class T>
    requires SpanConstructibleFrom<T&&> &&
             (!std::is_const_v<typename decltype(std::span(std::declval<T &&>()))::element_type>)
std::span<uint8_t> as_writable_u8_span(T&& t) {
    // Uses std::span's rules for vector/string/array/etc.
    auto s = std::span(std::forward<T>(t));
    auto b = std::as_writable_bytes(s);
    return {reinterpret_cast<uint8_t*>(b.data()), b.size()};
}

// Writable: single objects
template <class T>
    requires(!SpanConstructibleFrom<T&> && std::is_trivially_copyable_v<T>)
std::span<uint8_t> as_writable_u8_span(T& obj) noexcept {
    auto b = std::as_writable_bytes(std::span<T, 1>(&obj, 1));
    return {reinterpret_cast<uint8_t*>(b.data()), b.size()};
}

// Readable: anything span-constructible
template <class T>
    requires SpanConstructibleFrom<const T&>
std::span<const uint8_t> as_u8_span(const T& t) noexcept {
    // Uses std::span's rules for vector/string/array/etc.
    auto s = std::span(t);
    auto b = std::as_bytes(s);
    return {reinterpret_cast<const uint8_t*>(b.data()), b.size()};
}

// Readable: single objects
template <class T>
    requires(!SpanConstructibleFrom<const T&> && std::is_trivially_copyable_v<T>)
std::span<const uint8_t> as_u8_span(const T& obj) noexcept {
    auto b = std::as_bytes(std::span<const T, 1>(&obj, 1));
    return {reinterpret_cast<const uint8_t*>(b.data()), b.size()};
}

}  // namespace base
