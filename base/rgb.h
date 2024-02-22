#pragma once

#include <cstdint>

struct Rgb {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

namespace colors {
constexpr Rgb black{51, 51, 51};
constexpr Rgb yellow{249, 174, 88};
constexpr Rgb blue{102, 153, 204};
constexpr Rgb blue2{95, 180, 180};
constexpr Rgb green{128, 185, 121};
constexpr Rgb red{236, 95, 102};
constexpr Rgb red2{249, 123, 88};
constexpr Rgb red3{172, 122, 104};
constexpr Rgb grey2{153, 153, 153};
constexpr Rgb purple{198, 149, 198};
}
