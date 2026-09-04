#pragma once

#include <cstdint>

class color {
public:
    constexpr color() = default;

    static constexpr color from_normalised(float red, float green, float blue, float alpha) {
        return color(channel(red), channel(green), channel(blue), channel(alpha));
    }

    constexpr uint8_t red() const { return static_cast<uint8_t>(value_); }
    constexpr uint8_t green() const { return static_cast<uint8_t>(value_ >> 8); }
    constexpr uint8_t blue() const { return static_cast<uint8_t>(value_ >> 16); }
    constexpr uint8_t alpha() const { return static_cast<uint8_t>(value_ >> 24); }

    constexpr uint32_t packed_argb() const {
        return static_cast<uint32_t>(alpha()) << 24 | static_cast<uint32_t>(red()) << 16 |
               static_cast<uint32_t>(green()) << 8 | static_cast<uint32_t>(blue());
    }

    friend constexpr bool operator==(color, color) = default;

private:
    constexpr color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
        : value_(static_cast<uint32_t>(red) | static_cast<uint32_t>(green) << 8 |
                 static_cast<uint32_t>(blue) << 16 | static_cast<uint32_t>(alpha) << 24) {}

    static constexpr uint8_t channel(float value) {
        return static_cast<uint8_t>(static_cast<int32_t>(value * 255.0f + 0.5f));
    }

    uint32_t value_ = 0;
};

struct fcolor {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;

    constexpr fcolor() = default;
    constexpr fcolor(float red, float green, float blue, float alpha)
        : r(red), g(green), b(blue), a(alpha) {}
    constexpr fcolor(color value)
        : r(static_cast<float>(value.red()) / 255.0f),
          g(static_cast<float>(value.green()) / 255.0f),
          b(static_cast<float>(value.blue()) / 255.0f),
          a(static_cast<float>(value.alpha()) / 255.0f) {}

    constexpr operator color() const { return color::from_normalised(r, g, b, a); }

    constexpr uint32_t packed_argb() const { return static_cast<color>(*this).packed_argb(); }

    static constexpr fcolor from_packed_argb(uint32_t value) {
        return {static_cast<float>((value >> 16) & 0xff) / 255.0f,
                static_cast<float>((value >> 8) & 0xff) / 255.0f,
                static_cast<float>(value & 0xff) / 255.0f,
                static_cast<float>(value >> 24) / 255.0f};
    }
};

static_assert(sizeof(color) == 4);
static_assert(sizeof(fcolor) == 16);
