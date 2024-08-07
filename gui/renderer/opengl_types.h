#pragma once

#include <format>
#include <ostream>

namespace gui {

struct Vec2 {
    float x, y;
};
static_assert(sizeof(Vec2) == sizeof(float) * 2);

struct Vec3 {
    float x, y, z;
};
static_assert(sizeof(Vec3) == sizeof(float) * 3);

struct Vec4 {
    float x, y, z, w;
};
static_assert(sizeof(Vec4) == sizeof(float) * 4);

struct Rgb {
    uint8_t r, g, b;
};
static_assert(sizeof(Rgb) == sizeof(uint8_t) * 3);

struct Rgba {
    uint8_t r, g, b, a;

    static constexpr Rgba fromRgb(Rgb rgb, uint8_t a) {
        return Rgba{rgb.r, rgb.g, rgb.b, a};
    }
};
static_assert(sizeof(Rgba) == sizeof(uint8_t) * 4);

struct IVec4 {
    uint32_t x, y, z, w;
};
static_assert(sizeof(IVec4) == sizeof(uint32_t) * 4);

constexpr std::ostream& operator<<(std::ostream& out, const Vec2& vec) {
    return out << std::format("Vec2{{{}, {}}}", vec.x, vec.y);
}

constexpr std::ostream& operator<<(std::ostream& out, const Vec4& vec) {
    return out << std::format("Vec4{{{}, {}, {}, {}}}", vec.x, vec.y, vec.z, vec.w);
}

constexpr std::ostream& operator<<(std::ostream& out, const Rgba& color) {
    // `+` operator promotes uint8_t to a type printable as a number.
    // https://stackoverflow.com/a/28414758/14698275
    return out << std::format("Rgba{{{}, {}, {}, {}}}", +color.r, +color.g, +color.b, +color.a);
}

}
