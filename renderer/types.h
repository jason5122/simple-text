#pragma once

#include <cstddef>

namespace renderer {
struct CursorInfo {
    size_t byte;
    size_t line;
    size_t column;
    float x;
    float y;
};
}
