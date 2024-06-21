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
};

struct CaretInfo {
    size_t byte;
    size_t line;
    size_t column;
};

}
