#pragma once

#include <cstddef>

// TODO: See if we should put this under a namespace like "base::literals".
constexpr size_t operator""_Z(unsigned long long n) {
    return n;
}
