#pragma once

#include <cstddef>

namespace renderer {
struct Size {
    int width;
    int height;
};

struct Point {
    float x;
    float y;
};

struct CaretInfo {
    size_t byte;
    size_t line;
    size_t column;
    float x;
    float y;
};
}
