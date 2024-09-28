#include "base/rgb.h"
#include "color_scheme.h"

// TODO: Debug use; remove this.
#include "util/std_print.h"

namespace config {
ColorScheme::ColorScheme(bool dark_mode) {
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

    reload(dark_mode);
}

void ColorScheme::reload(bool dark_mode) {
    fs::path color_scheme_path = dark_mode ? dark_color_scheme_path : light_color_scheme_path;

    JsonSchema& schema = dark_mode ? kDefaultDarkSchema : kDefaultLightSchema;

    std::string buffer;
    if (fs::exists(color_scheme_path)) {
        // TODO: Handle errors in a better way.
        glz::parse_error error = glz::read_file_json(schema, color_scheme_path.string(), buffer);
        if (error) {
            std::println(glz::format_error(error, buffer));
        }
    } else {
        std::filesystem::create_directory(color_scheme_path.parent_path());
        glz::write_error error =
            glz::write_file_json<kDefaultOptions>(schema, color_scheme_path.string(), buffer);
        if (error) {
            std::println("Could not write color scheme to {}.", color_scheme_path);
        }
    }

    // TODO: Is there a better way to do this?
    if (!schema.foreground.empty()) foreground = base::ParseHexCode(schema.foreground);
    if (!schema.background.empty()) background = base::ParseHexCode(schema.background);
    if (!schema.caret.empty()) caret = base::ParseHexCode(schema.caret);
    if (!schema.tab_bar.empty()) tab_bar = base::ParseHexCode(schema.tab_bar);
    if (!schema.side_bar.empty()) side_bar = base::ParseHexCode(schema.side_bar);
    if (!schema.status_bar.empty()) status_bar = base::ParseHexCode(schema.status_bar);
    if (!schema.scroll_bar.empty()) scroll_bar = base::ParseHexCode(schema.scroll_bar);
}
}
