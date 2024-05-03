#include "base/filesystem/file_reader.h"
#include "base/rgb.h"
#include "color_scheme.h"
#include "glaze/glaze.hpp"
#include <iostream>

namespace config {
ColorScheme::ColorScheme() {
    kDefaultLightSchema = {
        .foreground = "#333333",
        .background = "#fdfdfd",
        .caret = "#5fb4b4",
        .tab_bar = "#bebebe",
        .side_bar = "#ebedef",
        .status_bar = "#c7cbd1",
        .scroll_bar = "#b6b6b6",
    };
    kDefaultDarkSchema = {
        .foreground = "#d8dee9",
        .background = "#303841",
        .caret = "#f9ae58",
        .tab_bar = "#4f565e",
        .side_bar = "#22262a",
        .status_bar = "#2e3238",
        .scroll_bar = "#6a7076",
    };

    reload();
}

void ColorScheme::reload() {
    std::cerr << "ColorScheme::reload()\n";

    fs::path data_dir = DataDir();
    // fs::path color_scheme_path = data_dir / "color_scheme_light.json";
    // fs::path color_scheme_path = data_dir / "color_scheme_dark.json";
    fs::path color_scheme_path;
    if (toggle) {
        color_scheme_path = data_dir / "color_scheme_light.json";
    } else {
        color_scheme_path = data_dir / "color_scheme_dark.json";
    }
    toggle = !toggle;

    // JsonSchema schema = kDefaultLightSchema;
    JsonSchema schema = kDefaultDarkSchema;

    std::string buffer;
    if (fs::exists(color_scheme_path)) {
        // TODO: Handle errors in a better way.
        glz::parse_error error = glz::read_file_json(schema, color_scheme_path.string(), buffer);
        if (error) {
            std::cerr << glz::format_error(error, buffer) << '\n';
        }
    } else {
        std::filesystem::create_directory(data_dir);
        glz::write_error error = glz::write_file_json(schema, color_scheme_path.string(), buffer);
        if (error) {
            std::cerr << "Could not write color scheme to " << color_scheme_path << ".\n";
        }
    }

    // TODO: Is there a better way to do this?
    if (!schema.foreground.empty()) foreground = ParseHexCode(schema.foreground);
    if (!schema.background.empty()) background = ParseHexCode(schema.background);
    if (!schema.caret.empty()) caret = ParseHexCode(schema.caret);
    if (!schema.tab_bar.empty()) tab_bar = ParseHexCode(schema.tab_bar);
    if (!schema.side_bar.empty()) side_bar = ParseHexCode(schema.side_bar);
    if (!schema.status_bar.empty()) status_bar = ParseHexCode(schema.status_bar);
    if (!schema.scroll_bar.empty()) scroll_bar = ParseHexCode(schema.scroll_bar);
}
}
