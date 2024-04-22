#pragma once

#include <cstdint>

struct Rgb {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

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

constexpr Rgb background{48, 56, 65};
constexpr Rgb cursor{yellow};
constexpr Rgb tab_bar{79, 86, 94};
constexpr Rgb side_bar{34, 38, 42};
constexpr Rgb status_bar{46, 50, 56};
constexpr Rgb scroll_bar{106, 112, 118};
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

constexpr Rgb background{253, 253, 253};
constexpr Rgb cursor{blue2};
constexpr Rgb tab_bar{190, 190, 190};
constexpr Rgb side_bar{235, 237, 239};
constexpr Rgb status_bar{199, 203, 209};
constexpr Rgb scroll_bar{182, 182, 182};
#endif
}
