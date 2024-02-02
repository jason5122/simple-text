#pragma once

#include <cstdint>

struct Rgb {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

constexpr Rgb BLACK{51, 51, 51};
constexpr Rgb YELLOW{249, 174, 88};
constexpr Rgb BLUE{102, 153, 204};
constexpr Rgb GREEN{128, 185, 121};
constexpr Rgb RED{236, 95, 102};
constexpr Rgb RED2{249, 123, 88};
constexpr Rgb GREY2{153, 153, 153};
constexpr Rgb PURPLE{198, 149, 198};
