#pragma once

#include <cstdint>
#include <string>

struct Rgb {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

inline Rgb ParseHexCode(std::string& hex) {
    uint8_t r = std::stoi(hex.substr(1, 2), nullptr, 16);
    uint8_t g = std::stoi(hex.substr(3, 2), nullptr, 16);
    uint8_t b = std::stoi(hex.substr(5, 2), nullptr, 16);
    return Rgb{r, g, b};
}

#define IS_DARK_MODE

namespace colors {
#ifdef IS_DARK_MODE
constexpr Rgb black{216, 222, 233};
constexpr Rgb yellow{249, 174, 88};
constexpr Rgb blue{102, 153, 204};
constexpr Rgb blue2{95, 180, 180};
constexpr Rgb green{153, 199, 148};
constexpr Rgb red{236, 95, 102};
constexpr Rgb red2{249, 123, 88};
constexpr Rgb red3{172, 122, 104};
constexpr Rgb grey2{153, 153, 153};
constexpr Rgb purple{198, 149, 198};
constexpr Rgb selection_focused{227, 230, 232};
constexpr Rgb selection_unfocused{235, 238, 239};
constexpr Rgb selection_border{212, 217, 221};
#else
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
constexpr Rgb selection_focused{227, 230, 232};
constexpr Rgb selection_unfocused{235, 238, 239};
constexpr Rgb selection_border{212, 217, 221};
#endif
}
