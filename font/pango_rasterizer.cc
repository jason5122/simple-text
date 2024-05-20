#include "build/buildflag.h"
#include "rasterizer.h"
#include <iostream>
#include <pango/pangocairo.h>
#include <vector>

namespace font {
class FontRasterizer::impl {
public:
    PangoFont* pango_font;
};

FontRasterizer::FontRasterizer() : pimpl{new impl{}} {}

bool FontRasterizer::setup(int id, std::string main_font_name, int font_size) {
    this->id = id;

    PangoFontMap* font_map = pango_cairo_font_map_get_default();
    PangoContext* context = pango_font_map_create_context(font_map);

    PangoFontDescription* desc = pango_font_description_new();
    pango_font_description_set_family(desc, main_font_name.c_str());
    pango_font_description_set_absolute_size(desc, font_size * PANGO_SCALE);
    if (!desc) {
        std::cerr << "pango_font_description_from_string() error.\n";
        return false;
    }

    pimpl->pango_font = pango_font_map_load_font(font_map, context, desc);
    if (!pimpl->pango_font) {
        std::cerr << "pango_font_map_load_font() error.\n";
        return false;
    }

    PangoFontMetrics* metrics = pango_font_get_metrics(pimpl->pango_font, nullptr);
    if (!metrics) {
        std::cerr << "pango_font_get_metrics() error.\n";
        return false;
    }

    int ascent = pango_font_metrics_get_ascent(metrics) / PANGO_SCALE;
    int descent = pango_font_metrics_get_descent(metrics) / PANGO_SCALE;
    int height = pango_font_metrics_get_height(metrics) / PANGO_SCALE;
    int line_height = std::max(ascent + descent, height);

    // TODO: Remove magic numbers that emulate Sublime Text.
    this->line_height = line_height + 2;
    this->descent = descent;

    return true;
}

cairo_t* create_layout_context() {
    cairo_surface_t* temp_surface;
    cairo_t* context;

    temp_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 0, 0);
    context = cairo_create(temp_surface);
    cairo_surface_destroy(temp_surface);

    return context;
}

cairo_t* create_cairo_context(int width, int height, int channels, cairo_surface_t** surf,
                              unsigned char** buffer) {
    *buffer = (unsigned char*)calloc(channels * width * height, sizeof(unsigned char));
    *surf = cairo_image_surface_create_for_data(*buffer, CAIRO_FORMAT_ARGB32, width, height,
                                                channels * width);
    return cairo_create(*surf);
}

// https://dthompson.us/posts/font-rendering-in-opengl-with-pango-and-cairo.html
RasterizedGlyph FontRasterizer::rasterizeUTF8(std::string_view utf8_str) {
    cairo_t* layout_context = create_layout_context();
    PangoLayout* layout = pango_cairo_create_layout(layout_context);
    pango_layout_set_text(layout, &utf8_str[0], utf8_str.length());

    PangoFontDescription* desc = pango_font_describe(pimpl->pango_font);
    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);

    int text_width;
    int text_height;
    pango_layout_get_size(layout, &text_width, &text_height);
    text_width /= PANGO_SCALE;
    text_height /= PANGO_SCALE;

    cairo_surface_t* surface;
    unsigned char* surface_data = nullptr;

    cairo_t* render_context =
        create_cairo_context(text_width, text_height, 4, &surface, &surface_data);

    cairo_set_source_rgba(render_context, 1, 1, 1, 1);
    pango_cairo_show_layout(render_context, layout);

    std::vector<uint8_t> temp_buffer;
    size_t pixels = text_width * text_height * 4;
    temp_buffer.reserve(pixels);
    for (size_t i = 0; i < pixels; i += 4) {
        temp_buffer.push_back(surface_data[i + 2]);
        temp_buffer.push_back(surface_data[i + 1]);
        temp_buffer.push_back(surface_data[i]);
        // temp_buffer.push_back(surface_data[i + 3]);
    }

    return RasterizedGlyph{
        .colored = false,
        .left = 0,
        .top = text_height,
        .width = text_width,
        .height = text_height,
        .advance = static_cast<float>(text_width),
        .buffer = temp_buffer,
    };
}

FontRasterizer::~FontRasterizer() {}
}
