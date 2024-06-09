#pragma once

namespace util {

// A base class to make a class non-copyable.
class NonCopyable {
protected:
    constexpr NonCopyable() = default;
    ~NonCopyable() = default;

    NonCopyable(NonCopyable&&) = default;
    NonCopyable& operator=(NonCopyable&&) = default;

private:
    NonCopyable(const NonCopyable&) = delete;
    void operator=(const NonCopyable&) = delete;
};

// A base class to make a class non-movable.
class NonMovable : NonCopyable {
protected:
    constexpr NonMovable() = default;
    ~NonMovable() = default;

private:
    NonMovable(NonMovable&&) = delete;
    void operator=(NonMovable&&) = delete;
};

}
