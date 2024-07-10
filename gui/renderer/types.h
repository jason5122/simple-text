#pragma once

#include "gui/renderer/opengl_types.h"
#include <cstddef>
#include <format>
#include <ostream>

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

    friend std::ostream& operator<<(std::ostream& out, const Size& s) {
        return out << std::format("Size{{{}, {}}}", s.width, s.height);
    }
};

struct Point {
    int x;
    int y;

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

    friend std::ostream& operator<<(std::ostream& out, const Point& p) {
        return out << std::format("Point{{{}, {}}}", p.x, p.y);
    }

    Vec2 toVec2() {
        return Vec2{static_cast<float>(x), static_cast<float>(y)};
    }
};

struct CaretInfo {
    size_t byte;
    size_t line;
    size_t column;
};

}
