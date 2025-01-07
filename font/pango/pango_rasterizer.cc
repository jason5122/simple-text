#include "font/font_rasterizer.h"

#include "font/pango/pango_helper.h"

#include <string>

// TODO: Debug use; remove this.
#include <algorithm>
#include <cassert>
#include <fmt/base.h>

namespace font {

struct FontRasterizer::NativeFontType {
    GObjectPtr<PangoFont> font;
};

class FontRasterizer::impl {
public:
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
        fmt::println("pango_font_map_load_font() error.");
        std::abort();
    }
    return cacheFont({std::move(pango_font)}, font_size);
}

size_t FontRasterizer::addSystemFont(int font_size, FontStyle style) {
    return addFont("system-ui", font_size, style);
}

size_t FontRasterizer::resizeFont(size_t font_id, int font_size) {
    PangoFont* font = font_id_to_native[font_id].font.get();
    PangoFontDescription* desc = pango_font_describe(font);

    PangoFontMap* font_map = pango_cairo_font_map_get_default();
    GObjectPtr<PangoContext> context{pango_font_map_create_context(font_map)};

    PangoFontDescription* desc_copy = pango_font_description_copy(desc);
    pango_font_description_set_size(desc_copy, font_size * PANGO_SCALE);
    GObjectPtr<PangoFont> pango_font{pango_font_map_load_font(font_map, context.get(), desc_copy)};
    if (!pango_font) {
        fmt::println("pango_font_map_load_font() error.");
        std::abort();
    }
    return cacheFont({std::move(pango_font)}, font_size);
}

RasterizedGlyph FontRasterizer::rasterize(size_t font_id, uint32_t glyph_id) const {
    PangoFont* font = font_id_to_native[font_id].font.get();

    PangoRectangle ink_rect;
    PangoRectangle logical_rect;
    pango_font_get_glyph_extents(font, glyph_id, &ink_rect, &logical_rect);
    int width = PANGO_PIXELS(logical_rect.width);
    int height = PANGO_PIXELS(logical_rect.height);
    int descent = PANGO_PIXELS(PANGO_DESCENT(logical_rect));
    // TODO: Do we need these?
    // int ascent = PANGO_PIXELS(PANGO_ASCENT(logical_rect));
    // int ink_width = PANGO_PIXELS(ink_rect.width);
    // int ink_height = PANGO_PIXELS(ink_rect.height);
    // int ink_top = PANGO_PIXELS(PANGO_ASCENT(ink_rect));
    // int ink_left = PANGO_PIXELS(PANGO_LBEARING(ink_rect));

    PangoGlyphStringPtr glyph_string{pango_glyph_string_new()};
    pango_glyph_string_set_size(glyph_string.get(), 1);

    PangoGlyphInfo gi = pimpl->glyph_info_cache[font_id][glyph_id];
    bool colored = gi.attr.is_color;
    glyph_string->glyphs[0] = std::move(gi);

    std::vector<uint8_t> bitmap_data(width * height * 4);
    CairoSurfacePtr surface{cairo_image_surface_create_for_data(
        bitmap_data.data(), CAIRO_FORMAT_ARGB32, width, height, width * 4)};
    CairoContextPtr render_context{cairo_create(surface.get())};

    cairo_translate(render_context.get(), 0, -descent);

    cairo_set_source_rgba(render_context.get(), 1, 1, 1, 1);
    pango_cairo_show_glyph_string(render_context.get(), font, glyph_string.get());

    return {
        .left = 0,
        .top = height - descent,
        .width = width,
        .height = height,
        .buffer = std::move(bitmap_data),
        .colored = colored,
    };
}

LineLayout FontRasterizer::layoutLine(size_t font_id, std::string_view str8) {
    assert(std::ranges::count(str8, '\n') == 0);

    PangoFont* font = font_id_to_native[font_id].font.get();

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

    int font_size = metrics(font_id).font_size;
    int line_height = metrics(font_id).line_height;

    int total_advance = 0;
    std::vector<ShapedGlyph> glyphs;
    for (GSList* run = layout_line->runs; run != nullptr; run = run->next) {
        PangoGlyphItem* glyph_item = static_cast<PangoGlyphItem*>(run->data);
        PangoItem* item = glyph_item->item;

        PangoFont* run_font = item->analysis.font;
        g_object_ref(run_font);
        GObjectPtr<PangoFont> run_font_ptr{run_font};
        size_t run_font_id = cacheFont({std::move(run_font_ptr)}, font_size);

        PangoGlyphString* glyph_string = glyph_item->glyphs;
        PangoGlyphInfo* glyph_infos = glyph_string->glyphs;
        int* log_clusters = glyph_string->log_clusters;

        int glyph_count = glyph_string->num_glyphs;
        int offset = item->offset;

        for (int i = 0; i < glyph_count; ++i) {
            PangoRectangle ink_rect;
            PangoRectangle logical_rect;
            pango_font_get_glyph_extents(run_font, glyph_infos[i].glyph, &ink_rect, &logical_rect);
            int width = PANGO_PIXELS(logical_rect.width);
            int height = PANGO_PIXELS(logical_rect.height);

            // Make some adjustments to glyph info struct.
            PangoGlyphInfo gi = glyph_infos[i];
            gi.geometry.width = PANGO_SCALE * width;
            gi.geometry.y_offset = PANGO_SCALE * height;

            // Cache glyph info struct.
            if (pimpl->glyph_info_cache.size() <= run_font_id) {
                pimpl->glyph_info_cache.resize(run_font_id + 1);
            }
            pimpl->glyph_info_cache[run_font_id][gi.glyph] = gi;

            const PangoGlyphGeometry& geometry = gi.geometry;
            int x_offset = PANGO_PIXELS(geometry.x_offset);
            int y_offset = PANGO_PIXELS(geometry.y_offset);

            // Pango's origin is at the top left. Invert the y-axis.
            // TODO: Since our app uses a top left origin, consider inverting bottom left origin
            // rasterizers (e.g., Core Text) instead of Pango.
            y_offset = line_height - y_offset;

            uint32_t glyph_id = gi.glyph;
            Point position = {.x = total_advance + x_offset, .y = y_offset};
            Point advance = {.x = width};
            size_t index = offset + log_clusters[i];

            ShapedGlyph glyph{
                .font_id = run_font_id,
                .glyph_id = glyph_id,
                .position = std::move(position),
                .advance = std::move(advance),
                .index = index,
            };
            glyphs.emplace_back(std::move(glyph));

            total_advance += advance.x;
        }
    }

    return {
        .layout_font_id = font_id,
        // We shouldn't use Pango's width since we make our own slight adjustments.
        .width = total_advance,
        .length = str8.length(),
        .glyphs = std::move(glyphs),
    };
}

size_t FontRasterizer::cacheFont(NativeFontType font, int font_size) {
    PangoFontDescription* run_font_desc = pango_font_describe(font.font.get());
    char* font_str = pango_font_description_to_string(run_font_desc);
    std::string font_name = font_str;
    g_free(font_str);

    // If the font is already present, return its ID.
    size_t hash = hashFont(font_name, font_size);
    if (auto it = font_hash_to_id.find(hash); it != font_hash_to_id.end()) {
        return it->second;
    }

    PangoFontMetricsPtr pango_metrics{pango_font_get_metrics(font.font.get(), nullptr)};
    if (!pango_metrics) {
        fmt::println("pango_font_get_metrics() error.");
        std::abort();
    }

    int ascent = pango_font_metrics_get_ascent(pango_metrics.get()) / PANGO_SCALE;
    int descent = pango_font_metrics_get_descent(pango_metrics.get()) / PANGO_SCALE;
    int height = pango_font_metrics_get_height(pango_metrics.get()) / PANGO_SCALE;

    // Round up to the next even number if odd.
    if (ascent % 2 == 1) ++ascent;
    if (descent % 2 == 1) ++descent;

    int line_height = std::max(ascent + descent, height);

    Metrics metrics{
        .line_height = line_height,
        .ascent = ascent,
        .descent = descent,
        .font_size = font_size,
    };

    size_t font_id = font_hash_to_id.size();
    font_hash_to_id.emplace(hash, font_id);
    font_id_to_native.emplace_back(std::move(font));
    font_id_to_metrics.emplace_back(std::move(metrics));
    return font_id;
}

}  // namespace font
