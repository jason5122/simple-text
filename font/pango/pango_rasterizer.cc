#include "font/pango/pango_helper.h"
#include "font/rasterizer.h"
#include <iostream>
#include <vector>

namespace font {

class FontRasterizer::impl {
public:
    GObjectPtr<PangoFont> pango_font;
};

FontRasterizer::FontRasterizer(const std::string& font_name_utf8, int font_size)
    : pimpl{new impl{}} {
    PangoFontMap* font_map = pango_cairo_font_map_get_default();
    GObjectPtr<PangoContext> context{pango_font_map_create_context(font_map)};

    PangoFontDescriptionPtr desc{pango_font_description_new()};
    pango_font_description_set_family(desc.get(), font_name_utf8.c_str());
    pango_font_description_set_size(desc.get(), font_size * PANGO_SCALE);
    if (!desc) {
        std::cerr << "pango_font_description_from_string() error.\n";
        // return false;
    }

    pimpl->pango_font =
        GObjectPtr<PangoFont>{pango_font_map_load_font(font_map, context.get(), desc.get())};
    if (!pimpl->pango_font) {
        std::cerr << "pango_font_map_load_font() error.\n";
        // return false;
    }

    PangoFontMetricsPtr metrics{pango_font_get_metrics(pimpl->pango_font.get(), nullptr)};
    if (!metrics) {
        std::cerr << "pango_font_get_metrics() error.\n";
        // return false;
    }

    int ascent = pango_font_metrics_get_ascent(metrics.get()) / PANGO_SCALE;
    int descent = pango_font_metrics_get_descent(metrics.get()) / PANGO_SCALE;
    int height = pango_font_metrics_get_height(metrics.get()) / PANGO_SCALE;
    int line_height = std::max(ascent + descent, height);

    // TODO: Remove magic numbers that emulate Sublime Text.
    this->line_height = line_height;
    this->descent = descent;
}

FontRasterizer::~FontRasterizer() {}

static inline CairoContextPtr CreateLayoutContext() {
    CairoSurfacePtr temp_surface{cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 0, 0)};
    return CairoContextPtr{cairo_create(temp_surface.get())};
}

static inline CairoContextPtr CreateRenderContext(int width, int height, int channels,
                                                  std::vector<unsigned char>& buffer) {
    CairoSurfacePtr surface{cairo_image_surface_create_for_data(&buffer[0], CAIRO_FORMAT_ARGB32,
                                                                width, height, channels * width)};
    return CairoContextPtr{cairo_create(surface.get())};
}

// https://dthompson.us/posts/font-rendering-in-opengl-with-pango-and-cairo.html
RasterizedGlyph FontRasterizer::rasterizeUTF8(std::string_view str8) {
    CairoContextPtr layout_context = CreateLayoutContext();
    GObjectPtr<PangoLayout> layout{pango_cairo_create_layout(layout_context.get())};
    pango_layout_set_text(layout.get(), &str8[0], str8.length());

    PangoFontDescriptionPtr desc{pango_font_describe(pimpl->pango_font.get())};
    pango_layout_set_font_description(layout.get(), desc.get());

    int text_width;
    int text_height;
    pango_layout_get_size(layout.get(), &text_width, &text_height);
    text_width /= PANGO_SCALE;
    text_height /= PANGO_SCALE;

    // TODO: Properly rasterize width like Sublime Text does (our widths are one pixel short).
    //       This hack makes monospaced fonts perfect, but ruins proportional fonts.
    // text_width += 1;

    std::vector<unsigned char> surface_data(text_width * text_height * 4);
    CairoContextPtr render_context = CreateRenderContext(text_width, text_height, 4, surface_data);

    cairo_set_source_rgba(render_context.get(), 0, 0, 0, 1);
    pango_cairo_show_layout(render_context.get(), layout.get());

    std::vector<uint8_t> temp_buffer;
    size_t pixels = text_width * text_height * 4;
    temp_buffer.reserve(pixels);
    for (size_t i = 0; i < pixels; i += 4) {
        temp_buffer.emplace_back(surface_data[i + 2]);
        temp_buffer.emplace_back(surface_data[i + 1]);
        temp_buffer.emplace_back(surface_data[i]);
        temp_buffer.emplace_back(surface_data[i + 3]);
    }

    return RasterizedGlyph{
        .colored = true,
        .left = 0,
        .top = text_height,
        .width = text_width,
        .height = text_height,
        .advance = text_width,
        .buffer = temp_buffer,
    };
}

}
