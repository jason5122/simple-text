#pragma once

#include <cstddef>

namespace renderer {
struct Size {
    int width;
    int height;
};

struct Point {
    int x;
    int y;
};

struct CaretInfo {
    size_t byte;
    size_t line;
    size_t column;
    int x;
    int y;
};
}
