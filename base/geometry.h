#pragma once

struct vec2 {
    double x = 0.0;
    double y = 0.0;
};

struct vec2i {
    int x = 0;
    int y = 0;
};

// Four doubles, matching the 0x20-byte stride of Sublime Text's dirty rectangles.
struct rect {
    double x = 0.0;
    double y = 0.0;
    double w = 0.0;
    double h = 0.0;

    constexpr double right() const { return x + w; }
    constexpr double bottom() const { return y + h; }
    constexpr bool empty() const { return w <= 0.0 || h <= 0.0; }
};

// Device-pixel rectangle stored as edges, matching Sublime Text's render contexts.
struct recti {
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;

    constexpr int width() const { return right - left; }
    constexpr int height() const { return bottom - top; }
    constexpr bool empty() const { return width() <= 0 || height() <= 0; }
};
