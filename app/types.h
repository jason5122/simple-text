#pragma once

#include <format>

namespace app {

enum class CursorStyle { kArrow, kIBeam };

struct Point {
    int x, y;

    friend Point operator+(const Point& p1, const Point& p2) {
        return {p1.x + p2.x, p1.y + p2.y};
    }

    Point& operator+=(const Point& rhs) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    friend Point operator-(const Point& p1, const Point& p2) {
        return {p1.x - p2.x, p1.y - p2.y};
    }

    Point& operator-=(const Point& rhs) {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

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

struct Size {
    int width;
    int height;

    friend Size operator+(const Size& s1, const Size& s2) {
        return {s1.width + s2.width, s1.height + s2.height};
    }

    Size& operator+=(const Size& rhs) {
        width += rhs.width;
        height += rhs.height;
        return *this;
    }

    friend Size operator-(const Size& s1, const Size& s2) {
        return {s1.width - s2.width, s1.height - s2.height};
    }

    Size& operator-=(const Size& rhs) {
        width -= rhs.width;
        height -= rhs.height;
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

template <>
struct std::formatter<app::Size> {
    constexpr auto parse(auto& ctx) {
        return ctx.begin();
    }

    auto format(const auto& s, auto& ctx) const {
        return std::format_to(ctx.out(), "Size({}, {})", s.width, s.height);
    }
};
