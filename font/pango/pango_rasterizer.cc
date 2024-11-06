#include "font/font_rasterizer.h"
#include "font/pango/pango_helper.h"
#include <cairo-ft.h>
#include <unordered_map>
#include <vector>

// TODO: Debug use; remove this.
#include "util/std_print.h"
#include <cassert>
#include <ranges>

namespace font {

class FontRasterizer::impl {
public:
    std::unordered_map<std::string, size_t> font_postscript_name_to_id;
    std::vector<GObjectPtr<PangoFont>> font_id_to_native;
    std::vector<Metrics> font_id_to_metrics;
    size_t cacheFont(PangoFont* font);

    std::vector<std::unordered_map<PangoGlyph, PangoGlyphInfo>> glyph_info_cache;
};

FontRasterizer::FontRasterizer() : pimpl{new impl{}} {}

FontRasterizer::~FontRasterizer() {}

size_t FontRasterizer::addFont(std::string_view font_name_utf8, int font_size, FontStyle style) {
    PangoFontMap* font_map = pango_cairo_font_map_get_default();
    GObjectPtr<PangoContext> context{pango_font_map_create_context(font_map)};

    PangoFontDescriptionPtr desc{pango_font_description_new()};
    pango_font_description_set_family_static(desc.get(), font_name_utf8.data());
    pango_font_description_set_size(desc.get(), font_size * PANGO_SCALE);

    if ((style & FontStyle::kBold) != FontStyle::kNone) {
        pango_font_description_set_weight(desc.get(), PANGO_WEIGHT_BOLD);
    }
    if ((style & FontStyle::kItalic) != FontStyle::kNone) {
        pango_font_description_set_style(desc.get(), PANGO_STYLE_ITALIC);
    }

    GObjectPtr<PangoFont> pango_font{
        pango_font_map_load_font(font_map, context.get(), desc.get())};
    if (!pango_font) {
        std::println("pango_font_map_load_font() error.");
        std::abort();
    }

    return pimpl->cacheFont(pango_font.get());
}

const Metrics& FontRasterizer::getMetrics(size_t font_id) const {
    return pimpl->font_id_to_metrics.at(font_id);
}

RasterizedGlyph FontRasterizer::rasterize(size_t font_id, uint32_t glyph_id) const {
    PangoFont* font = pimpl->font_id_to_native[font_id].get();

    PangoRectangle ink_rect;
    PangoRectangle logical_rect;
    pango_font_get_glyph_extents(font, glyph_id, &ink_rect, &logical_rect);
    int width = PANGO_PIXELS(logical_rect.width);
    int height = PANGO_PIXELS(logical_rect.height);

    PangoGlyphStringPtr glyph_string{pango_glyph_string_new()};
    pango_glyph_string_set_size(glyph_string.get(), 1);

    PangoGlyphInfo gi = pimpl->glyph_info_cache[font_id][glyph_id];
    bool colored = gi.attr.is_color;
    glyph_string->glyphs[0] = std::move(gi);

    std::vector<uint8_t> surface_data(width * height * 4);
    CairoSurfacePtr surface{cairo_image_surface_create_for_data(
        surface_data.data(), CAIRO_FORMAT_ARGB32, width, height, width * 4)};
    CairoContextPtr render_context{cairo_create(surface.get())};

    int descent = getMetrics(font_id).descent;
    cairo_translate(render_context.get(), 0, -descent);

    cairo_set_source_rgba(render_context.get(), 1, 1, 1, 1);
    pango_cairo_show_glyph_string(render_context.get(), font, glyph_string.get());

    std::vector<uint8_t> buffer;
    size_t pixels = width * height * 4;
    buffer.reserve(pixels);
    for (size_t i = 0; i < pixels; i += 4) {
        buffer.emplace_back(surface_data[i + 2]);
        buffer.emplace_back(surface_data[i + 1]);
        buffer.emplace_back(surface_data[i]);
        if (colored) {
            buffer.emplace_back(surface_data[i + 3]);
        }
    }

    return {
        .colored = colored,
        .left = 0,
        .top = height - descent,
        .width = width,
        .height = height,
        .buffer = std::move(buffer),
    };
}

LineLayout FontRasterizer::layoutLine(size_t font_id, std::string_view str8) const {
    assert(std::ranges::count(str8, '\n') == 0);

    PangoFont* font = pimpl->font_id_to_native[font_id].get();

    CairoSurfacePtr temp_surface{cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 0, 0)};
    CairoContextPtr layout_context{cairo_create(temp_surface.get())};

    cairo_font_options_t* font_options = cairo_font_options_create();
    cairo_font_options_set_antialias(font_options, CAIRO_ANTIALIAS_SUBPIXEL);
    cairo_set_font_options(layout_context.get(), font_options);
    cairo_font_options_destroy(font_options);

    GObjectPtr<PangoLayout> layout{pango_cairo_create_layout(layout_context.get())};
    pango_layout_set_text(layout.get(), str8.data(), str8.length());

    PangoFontDescriptionPtr desc{pango_font_describe(font)};
    pango_layout_set_font_description(layout.get(), desc.get());

    // We don't need to free this. This is owned by the `PangoLayout` instance.
    PangoLayoutLine* layout_line = pango_layout_get_line_readonly(layout.get(), 0);

    int total_advance = 0;
    std::vector<ShapedRun> runs;
    for (GSList* run = layout_line->runs; run != nullptr; run = run->next) {
        PangoGlyphItem* glyph_item = static_cast<PangoGlyphItem*>(run->data);
        PangoItem* item = glyph_item->item;

        PangoFont* run_font = item->analysis.font;
        size_t run_font_id = pimpl->cacheFont(run_font);

        PangoGlyphString* glyph_string = glyph_item->glyphs;
        PangoGlyphInfo* glyph_infos = glyph_string->glyphs;
        int* log_clusters = glyph_string->log_clusters;

        int glyph_count = glyph_string->num_glyphs;
        int offset = item->offset;

        std::vector<ShapedGlyph> glyphs;
        glyphs.reserve(glyph_count);
        for (int i = 0; i < glyph_count; i++) {
            PangoRectangle ink_rect;
            PangoRectangle logical_rect;
            pango_font_get_glyph_extents(run_font, glyph_infos[i].glyph, &ink_rect, &logical_rect);
            int width = PANGO_PIXELS(logical_rect.width);
            int height = PANGO_PIXELS(logical_rect.height);

            // Seems like Sublime Text adds some pixels?
            // width += 1;
            // height += 2;

            // Make some adjustments to glyph info struct.
            PangoGlyphInfo gi = glyph_infos[i];
            gi.geometry.width = PANGO_SCALE * width;
            gi.geometry.y_offset = PANGO_SCALE * height;

            // Cache glyph info struct.
            // TODO: Refactor this ugly hack.
            while (pimpl->glyph_info_cache.size() <= run_font_id) {
                pimpl->glyph_info_cache.emplace_back();
            }
            pimpl->glyph_info_cache[run_font_id][gi.glyph] = gi;

            const PangoGlyphGeometry& geometry = gi.geometry;
            int x_offset = PANGO_PIXELS(geometry.x_offset);
            int y_offset = PANGO_PIXELS(geometry.y_offset);

            uint32_t glyph_id = gi.glyph;
            Point position = {.x = total_advance + x_offset, .y = y_offset};
            Point advance = {.x = width};
            size_t index = offset + log_clusters[i];

            glyphs.emplace_back(
                ShapedGlyph{glyph_id, std::move(position), std::move(advance), index});

            total_advance += advance.x;
        }

        runs.emplace_back(ShapedRun{run_font_id, std::move(glyphs)});
    }

    // Fetch ascent from the main line layout font. Otherwise, the baseline will shift up and down
    // when fonts with different ascents mix (e.g., emoji being taller than plain text).
    int ascent = getMetrics(font_id).ascent;

    return {
        .layout_font_id = font_id,
        // We shouldn't use Pango's width since we make our own slight adjustments.
        .width = total_advance,
        .length = str8.length(),
        .runs = std::move(runs),
        .ascent = ascent,
    };
}

size_t FontRasterizer::impl::cacheFont(PangoFont* font) {
    PangoFontDescription* run_font_desc = pango_font_describe(font);
    char* font_str = pango_font_description_to_string(run_font_desc);
    std::string font_name = font_str;
    g_free(font_str);

    if (!font_postscript_name_to_id.contains(font_name)) {
        // TODO: Incorporate this into a smart pointer.
        g_object_ref(font);

        PangoFontMetricsPtr pango_metrics{pango_font_get_metrics(font, nullptr)};
        if (!pango_metrics) {
            std::println("pango_font_get_metrics() error.");
            std::abort();
        }

        int ascent = pango_font_metrics_get_ascent(pango_metrics.get()) / PANGO_SCALE;
        int descent = pango_font_metrics_get_descent(pango_metrics.get()) / PANGO_SCALE;
        int height = pango_font_metrics_get_height(pango_metrics.get()) / PANGO_SCALE;

        // Round up to the next even number if odd.
        if (ascent % 2 == 1) ascent += 1;
        if (descent % 2 == 1) descent += 1;

        int line_height = std::max(ascent + descent, height);

        Metrics metrics{
            .line_height = line_height,
            .descent = descent,
            .ascent = ascent,
        };

        size_t font_id = font_id_to_native.size();
        font_postscript_name_to_id.emplace(font_name, font_id);
        font_id_to_native.emplace_back(font);
        font_id_to_metrics.emplace_back(std::move(metrics));
    }
    return font_postscript_name_to_id.at(font_name);
}

}  // namespace font
