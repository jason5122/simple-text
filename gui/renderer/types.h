#pragma once

#include "app/types.h"
#include "gui/renderer/opengl_types.h"
#include <cstddef>
#include <format>

namespace gui {

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

// struct Point {
//     int x;
//     int y;

//     friend Point operator+(const Point& p1, const Point& p2) {
//         return {p1.x + p2.x, p1.y + p2.y};
//     }

//     Point& operator+=(const Point& rhs) {
//         x += rhs.x;
//         y += rhs.y;
//         return *this;
//     }

//     friend Point operator-(const Point& p1, const Point& p2) {
//         return {p1.x - p2.x, p1.y - p2.y};
//     }

//     Point& operator-=(const Point& rhs) {
//         x -= rhs.x;
//         y -= rhs.y;
//         return *this;
//     }

//     constexpr Vec2 toVec2() {
//         return Vec2{static_cast<float>(x), static_cast<float>(y)};
//     }

//     static constexpr std::optional<Point> fromAppPoint(const std::optional<app::Point>& p) {
//         if (!p) return std::nullopt;
//         return Point{p.value().x, p.value().y};
//     }

//     static constexpr Point fromAppPoint(const app::Point& p) {
//         return Point{p.x, p.y};
//     }
// };

}  // namespace gui

template <>
struct std::formatter<gui::Size> {
    constexpr auto parse(auto& ctx) {
        return ctx.begin();
    }

    auto format(const auto& s, auto& ctx) const {
        return std::format_to(ctx.out(), "Size({}, {})", s.width, s.height);
    }
};

// template <>
// struct std::formatter<gui::Point> {
//     constexpr auto parse(auto& ctx) {
//         return ctx.begin();
//     }

//     auto format(const auto& p, auto& ctx) const {
//         return std::format_to(ctx.out(), "Point({}, {})", p.x, p.y);
//     }
// };
