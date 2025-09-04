#pragma once

#include <cstdint>
#include <tree_sitter/api.h>

constexpr bool operator==(const TSPoint& p1, const TSPoint& p2) {
    return p1.row == p2.row && p1.column == p2.column;
}
constexpr auto operator<=>(const TSPoint& p1, const TSPoint& p2) {
    if (auto c = p1.row <=> p2.row; c != 0) return c;
    return p1.column <=> p2.column;
}

namespace highlight {

struct Highlight {
    TSPoint start;
    TSPoint end;
    size_t capture_index;

    constexpr bool contains(const TSPoint& p) const {
        return start <= p && p < end;
    }
    friend constexpr bool operator==(const Highlight& h1, const Highlight& h2) {
        return h1.start == h2.start && h1.end == h2.end;
    }
    friend constexpr bool operator!=(const Highlight& h1, const Highlight& h2) {
        return !(h1 == h2);
    }
};

struct Rgb {
    uint8_t r, g, b;
};

}  // namespace highlight
