#pragma once

#include "base/rgb.h"

namespace config {
class ColorScheme {
public:
    Rgb foreground{};
    Rgb background{};
    Rgb caret{};
    Rgb tab_bar{};
    Rgb side_bar{};
    Rgb status_bar{};
    Rgb scroll_bar{};

    ColorScheme();
};
}
