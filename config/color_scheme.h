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

private:
    struct JsonSchema {
        std::string foreground;
        std::string background;
        std::string caret;
        std::string tab_bar;
        std::string side_bar;
        std::string status_bar;
        std::string scroll_bar;
    };

    // TODO: Make these static and constexpr. This requires rethinking how we store the fields
    // since we can't use std::string for constexpr.
    JsonSchema kDefaultLightSchema;
    JsonSchema kDefaultDarkSchema;
};
}
