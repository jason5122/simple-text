#pragma once

template <typename T> T max(T x, T y) {
    return x < y ? y : x;
    // return x;
}
