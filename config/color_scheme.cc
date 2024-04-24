#include "base/filesystem/file_reader.h"
#include "base/rgb.h"
#include "color_scheme.h"
#include "glaze/glaze.hpp"
#include <iostream>

namespace config {
struct Schema {
    std::string foreground;
    std::string background;
    std::string caret;
    std::string tab_bar;
    std::string side_bar;
    std::string status_bar;
    std::string scroll_bar;
};

ColorScheme::ColorScheme() {
    // fs::path color_scheme_path = DataPath() / "color_scheme_light.json";
    fs::path color_scheme_path = DataPath() / "color_scheme_dark.json";

    glz::parse_error error;

    Schema schema;
    std::string buffer;

    // TODO: Handle errors in a better way.
    error = glz::read_file_json(schema, color_scheme_path.string(), buffer);
    if (error) {
        std::cerr << glz::format_error(error, buffer) << '\n';
    }

    // TODO: Is there a better way to do this?
    if (!schema.foreground.empty()) {
        foreground = ParseHexCode(schema.foreground);
    }
    if (!schema.background.empty()) {
        background = ParseHexCode(schema.background);
    }
    if (!schema.caret.empty()) {
        caret = ParseHexCode(schema.caret);
    }
    if (!schema.tab_bar.empty()) {
        tab_bar = ParseHexCode(schema.tab_bar);
    }
    if (!schema.side_bar.empty()) {
        side_bar = ParseHexCode(schema.side_bar);
    }
    if (!schema.status_bar.empty()) {
        status_bar = ParseHexCode(schema.status_bar);
    }
    if (!schema.scroll_bar.empty()) {
        scroll_bar = ParseHexCode(schema.scroll_bar);
    }
}
}
