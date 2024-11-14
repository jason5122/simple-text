#pragma once

#include <format>
#include <tree_sitter/api.h>

constexpr bool operator==(const TSPoint& p1, const TSPoint& p2) {
    return p1.row == p2.row && p1.column == p2.column;
}
constexpr bool operator!=(const TSPoint& p1, const TSPoint& p2) {
    return !(p1 == p2);
}
constexpr bool operator<(const TSPoint& p1, const TSPoint& p2) {
    if (p1.row == p2.row) {
        return p1.column < p2.column;
    } else {
        return p1.row < p2.row;
    }
}
constexpr bool operator>(const TSPoint& p1, const TSPoint& p2) {
    return p2 < p1;
}
constexpr bool operator<=(const TSPoint& p1, const TSPoint& p2) {
    return !(p1 > p2);
}
constexpr bool operator>=(const TSPoint& p1, const TSPoint& p2) {
    return !(p1 < p2);
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

template <>
struct std::formatter<TSPoint> {
    constexpr auto parse(auto& ctx) {
        return ctx.begin();
    }

    auto format(const auto& p, auto& ctx) const {
        return std::format_to(ctx.out(), "TSPoint({}, {})", p.row, p.column);
    }
};

template <>
struct std::formatter<highlight::Rgb> {
    constexpr auto parse(auto& ctx) {
        return ctx.begin();
    }

    auto format(const auto& rgb, auto& ctx) const {
        return std::format_to(ctx.out(), "Rgb({}, {}, {})", rgb.r, rgb.g, rgb.b);
    }
};
