#pragma once

namespace ui {

struct Point {
    float x = 0;
    float y = 0;
};

struct Size {
    float width = 0;
    float height = 0;
};

struct Rect {
    float x = 0;
    float y = 0;
    float width = 0;
    float height = 0;

    bool contains(Point point) const {
        return point.x >= x && point.x < x + width && point.y >= y && point.y < y + height;
    }
};

struct Color {
    float r = 0;
    float g = 0;
    float b = 0;
    float a = 1;
};

}  // namespace ui
