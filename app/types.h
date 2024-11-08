#pragma once

#include <format>

namespace app {

enum class CursorStyle { kArrow, kIBeam };

struct Point {
    int x, y;

    friend constexpr Point operator*(const Point& p, int val) {
        return {p.x * val, p.y * val};
    }

    friend constexpr Point operator*(int val, const Point& p) {
        return p * val;
    }

    constexpr Point& operator*=(int val) {
        x *= val;
        y *= val;
        return *this;
    }
};

struct Delta {
    int dx, dy;

    friend constexpr Delta operator*(const Delta& d, int val) {
        return {d.dx * val, d.dy * val};
    }

    friend constexpr Delta operator*(int val, const Delta& d) {
        return d * val;
    }

    constexpr Delta& operator*=(int val) {
        dx *= val;
        dy *= val;
        return *this;
    }
};

}  // namespace app

template <>
struct std::formatter<app::Point> {
    constexpr auto parse(auto& ctx) {
        return ctx.begin();
    }

    auto format(const auto& p, auto& ctx) const {
        return std::format_to(ctx.out(), "Point({}, {})", p.x, p.y);
    }
};
