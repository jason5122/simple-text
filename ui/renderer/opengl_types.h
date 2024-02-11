#pragma once

struct Vec2 {
    float x, y;
};
static_assert(sizeof(Vec2) == sizeof(float) * 2);

struct Vec4 {
    float x, y, z, w;
};
static_assert(sizeof(Vec4) == sizeof(float) * 4);

struct Rgba {
    uint8_t r, g, b, a;
};
static_assert(sizeof(Rgba) == sizeof(uint8_t) * 4);
