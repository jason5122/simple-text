#pragma once

#include "base/filesystem/file_reader.h"
#include "base/rgb.h"
#include "glaze/glaze.hpp"

namespace config {
class ColorScheme {
public:
    base::Rgb foreground{};
    base::Rgb background{};
    base::Rgb caret{};
    base::Rgb tab_bar{};
    base::Rgb side_bar{};
    base::Rgb status_bar{};
    base::Rgb scroll_bar{};

    ColorScheme(bool dark_mode);
    void reload(bool dark_mode);

private:
    static inline const fs::path light_color_scheme_path = DataDir() / "color_scheme_light.json";
    static inline const fs::path dark_color_scheme_path = DataDir() / "color_scheme_dark.json";
    static constexpr glz::opts kDefaultOptions{.prettify = true, .indentation_width = 2};

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
