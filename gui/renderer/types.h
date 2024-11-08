#pragma once

#include "app/types.h"
#include "gui/renderer/opengl_types.h"
#include <cstddef>
#include <format>

namespace gui {

// struct Size {
//     int width;
//     int height;

//     friend Size operator+(const Size& s1, const Size& s2) {
//         return {s1.width + s2.width, s1.height + s2.height};
//     }

//     Size& operator+=(const Size& rhs) {
//         width += rhs.width;
//         height += rhs.height;
//         return *this;
//     }

//     friend Size operator-(const Size& s1, const Size& s2) {
//         return {s1.width - s2.width, s1.height - s2.height};
//     }

//     Size& operator-=(const Size& rhs) {
//         width -= rhs.width;
//         height -= rhs.height;
//         return *this;
//     }
// };

}  // namespace gui

// template <>
// struct std::formatter<gui::Size> {
//     constexpr auto parse(auto& ctx) {
//         return ctx.begin();
//     }

//     auto format(const auto& s, auto& ctx) const {
//         return std::format_to(ctx.out(), "Size({}, {})", s.width, s.height);
//     }
// };

// template <>
// struct std::formatter<gui::Point> {
//     constexpr auto parse(auto& ctx) {
//         return ctx.begin();
//     }

//     auto format(const auto& p, auto& ctx) const {
//         return std::format_to(ctx.out(), "Point({}, {})", p.x, p.y);
//     }
// };
