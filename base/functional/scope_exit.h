#pragma once

#include <concepts>
#include <utility>

namespace base {

template <std::invocable F>
class ScopeExit {
public:
    template <std::invocable T>
    explicit constexpr ScopeExit(T&& scope_exit_func)
        : scope_exit_func{std::forward<T>(scope_exit_func)} {}

    ~ScopeExit() { scope_exit_func(); }

private:
    F scope_exit_func;
};

template <std::invocable F>
ScopeExit(F&&) -> ScopeExit<F>;

}  // namespace base
