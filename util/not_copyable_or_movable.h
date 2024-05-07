#pragma once

#define NOT_COPYABLE(T)                                                                           \
    T(T const&) = delete;                                                                         \
    T& operator=(T const&) = delete;

#define NOT_MOVABLE(T)                                                                            \
    T(T&&) = delete;                                                                              \
    T& operator=(T&&) = delete;
