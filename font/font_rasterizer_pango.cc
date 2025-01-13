#include "font/font_rasterizer.h"

#include <memory>
#include <string>

#include <pango/pangocairo.h>

// TODO: Debug use; remove this.
#include <cassert>
#include <fmt/base.h>

namespace font {

template <typename T, auto fn>
struct Deleter {
    void operator()(T* ptr) {
        fn(ptr);
    }
};

template <typename T>
using GObjectPtr = std::unique_ptr<T, Deleter<T, g_object_unref>>;
template <typename T, auto fn>
using UniquePtrDeleter = std::unique_ptr<T, Deleter<T, fn>>;

using PangoFontDescriptionPtr =
    UniquePtrDeleter<PangoFontDescription, pango_font_description_free>;
using PangoFontMetricsPtr = UniquePtrDeleter<PangoFontMetrics, pango_font_metrics_unref>;
using PangoGlyphStringPtr = UniquePtrDeleter<PangoGlyphString, pango_glyph_string_free>;
using CairoSurfacePtr = UniquePtrDeleter<cairo_surface_t, cairo_surface_destroy>;
using CairoContextPtr = UniquePtrDeleter<cairo_t, cairo_destroy>;

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
    int ascent = PANGO_PIXELS(PANGO_ASCENT(logical_rect));
    int top = ascent;
    // TODO: Do we need these?
    int ink_width = PANGO_PIXELS(ink_rect.width);
    int ink_height = PANGO_PIXELS(ink_rect.height);
    int ink_top = PANGO_PIXELS(PANGO_ASCENT(ink_rect));
    int ink_left = PANGO_PIXELS(PANGO_LBEARING(ink_rect));
    int ink_descent = PANGO_PIXELS(PANGO_DESCENT(ink_rect));

    // width = ink_width;
    // height = ink_height;
    // descent = ink_descent;
    // top = ink_top;

    if (font_id == 1 || font_id == 2 || font_id == 3) return {};

    fmt::println("font = {}, glyph = {}, width = {}, height = {}, descent = {}, ascent = {}",
                 font_id, glyph_id, width, height, descent, ascent);
    fmt::println("ink: {} {} {} {} {}", ink_width, ink_height, ink_left, ink_top, ink_descent);

    // TODO: Don't hard-code this.
    int scale_factor = 2;
    width *= scale_factor;
    height *= scale_factor;
    descent *= scale_factor;
    top *= scale_factor;

    auto glyph_string = PangoGlyphStringPtr{pango_glyph_string_new()};
    pango_glyph_string_set_size(glyph_string.get(), 1);

    PangoGlyphInfo gi = pimpl->glyph_info_cache[font_id][glyph_id];
    bool colored = gi.attr.is_color;
    glyph_string->glyphs[0] = std::move(gi);

    std::vector<uint8_t> bitmap_data(height * width * 4);
    auto surface = CairoSurfacePtr{cairo_image_surface_create_for_data(
        bitmap_data.data(), CAIRO_FORMAT_ARGB32, width, height, width * 4)};
    auto context = CairoContextPtr{cairo_create(surface.get())};

    // int val = PANGO_PIXELS(PANGO_ASCENT(logical_rect));
    // val *= -1;
    // val -= ink_top;
    // fmt::println("val = {}", val);
    // int left = 1;
    // left *= scale_factor;
    // cairo_translate(context.get(), -left, -(descent + ascent));
    // cairo_translate(context.get(), -2 * 2, val);
    cairo_translate(context.get(), 0, -descent);
    cairo_scale(context.get(), scale_factor, scale_factor);

    cairo_set_source_rgba(context.get(), 1, 1, 1, 1);
    pango_cairo_show_glyph_string(context.get(), font, glyph_string.get());

    return {
        .left = 0,
        .top = top,
        .width = width,
        .height = height,
        .buffer = std::move(bitmap_data),
        .colored = colored,
    };
}

LineLayout FontRasterizer::layoutLine(size_t font_id, std::string_view str8) {
    assert(str8.find('\n') == std::string_view::npos);

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
            gi.geometry.width = width * PANGO_SCALE;
            gi.geometry.y_offset = height * PANGO_SCALE;

            // Cache glyph info struct.
            if (pimpl->glyph_info_cache.size() <= run_font_id) {
                pimpl->glyph_info_cache.resize(run_font_id + 1);
            }
            pimpl->glyph_info_cache[run_font_id][gi.glyph] = gi;

            const PangoGlyphGeometry& geometry = gi.geometry;
            int x_offset = PANGO_PIXELS(geometry.x_offset);
            int y_offset = PANGO_PIXELS(geometry.y_offset);

            // TODO: Don't hard-code this.
            int scale_factor = 2;
            width *= scale_factor;
            height *= scale_factor;
            x_offset *= scale_factor;
            y_offset *= scale_factor;

            // Pango's origin is at the top left. Invert the y-axis.
            // TODO: Since our app uses a top left origin, consider inverting bottom left origin
            // rasterizers (e.g., Core Text) instead of Pango.
            y_offset = line_height - y_offset;

            uint32_t glyph_id = gi.glyph;
            Point position = {.x = total_advance + x_offset, .y = y_offset};
            Point advance = {.x = width};
            size_t index = offset + log_clusters[i];

            ShapedGlyph glyph = {
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

    // TODO: Don't hard-code this.
    int scale_factor = 2;

    int ascent = pango_font_metrics_get_ascent(pango_metrics.get()) / PANGO_SCALE;
    int descent = pango_font_metrics_get_descent(pango_metrics.get()) / PANGO_SCALE;
    int height = pango_font_metrics_get_height(pango_metrics.get()) / PANGO_SCALE;
    ascent *= scale_factor;
    descent *= scale_factor;
    height *= scale_factor;

    int line_height = std::max(ascent + descent, height);

    Metrics metrics = {
        .line_height = line_height,
        .ascent = ascent,
        .descent = descent,
        .font_size = font_size,
    };

    size_t font_id = font_hash_to_id.size();
    font_hash_to_id.emplace(hash, font_id);
    font_id_to_native.emplace_back(std::move(font));
    font_id_to_metrics.emplace_back(std::move(metrics));
    font_id_to_postscript_name.emplace_back(std::move(font_name));
    return font_id;
}

}  // namespace font
