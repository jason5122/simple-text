#include "fx/fx.h"

#include "base/check.h"
#include "base/unicode/unicode.h"

#include <cairo-ft.h>
#include <cmath>
#include <limits>
#include <memory>
#include <pango/pangocairo.h>
#include <string>
#include <utility>
#include <vector>

namespace {

template <typename T, auto Destroy>
struct native_deleter {
    void operator()(T* value) const {
        if (value) Destroy(value);
    }
};

template <typename T>
using g_object_ptr = std::unique_ptr<T, native_deleter<T, g_object_unref>>;
using pango_description_ptr =
    std::unique_ptr<PangoFontDescription,
                    native_deleter<PangoFontDescription, pango_font_description_free>>;
using pango_metrics_ptr =
    std::unique_ptr<PangoFontMetrics, native_deleter<PangoFontMetrics, pango_font_metrics_unref>>;
using pango_iterator_ptr =
    std::unique_ptr<PangoLayoutIter, native_deleter<PangoLayoutIter, pango_layout_iter_free>>;
using pango_attribute_list_ptr =
    std::unique_ptr<PangoAttrList, native_deleter<PangoAttrList, pango_attr_list_unref>>;
using cairo_font_options_ptr =
    std::unique_ptr<cairo_font_options_t,
                    native_deleter<cairo_font_options_t, cairo_font_options_destroy>>;
using cairo_surface_ptr =
    std::unique_ptr<cairo_surface_t, native_deleter<cairo_surface_t, cairo_surface_destroy>>;
using cairo_context_ptr = std::unique_ptr<cairo_t, native_deleter<cairo_t, cairo_destroy>>;

const fx_gamma_ramp* identity_gamma_ramp() {
    static const fx_gamma_ramp ramp = [] {
        fx_gamma_ramp result;
        for (size_t i = 0; i < result.values.size(); ++i) {
            result.values[i] = static_cast<uint8_t>(i);
            result.inverse_values[i] = static_cast<uint8_t>(i);
        }
        return result;
    }();
    return &ramp;
}

std::string font_features(uint32_t attrs) {
    std::string result;
    auto feature = [&result](std::string_view name, bool enabled) {
        if (!result.empty()) result += ", ";
        result += '"';
        result += name;
        result += enabled ? "\" 1" : "\" 0";
    };
    feature("liga", !(attrs & FX_FONT_NO_LIGA));
    feature("clig", !(attrs & FX_FONT_NO_CLIG));
    feature("calt", !(attrs & FX_FONT_NO_CALT));
    if (attrs & FX_FONT_DLIG) feature("dlig", true);
    for (uint32_t i = 0; i < 10; ++i) {
        if (!(attrs & (FX_FONT_SS01 << i))) continue;
        const char name[] = {'s', 's', static_cast<char>('0' + (i + 1) / 10),
                             static_cast<char>('0' + (i + 1) % 10), '\0'};
        feature(name, true);
    }
    return result;
}

class pango_font final : public fx_font {
public:
    static std::unique_ptr<pango_font> create(std::string family, float size, uint32_t attrs);

    uint32_t attrs() const override { return attrs_; }
    fx_font_metrics metrics() const override;
    float raster_ascent() const override { return metrics().ascent; }
    std::unique_ptr<fx_layout> shape(std::string_view utf8) override;
    std::unique_ptr<fx_layout> shape(std::u32string_view utf32) override {
        return shape(base::utf32_to_utf8(utf32));
    }
    void extents(uint32_t glyph, float scale, vec2& origin, vec2& size) override;
    void rasterize(uint32_t glyph,
                   vec2 position,
                   float scale,
                   fx_pixel_buffer* buffer,
                   color foreground,
                   uint32_t subpixel_order) override;
    bool is_color_glyph(uint32_t glyph) override;
    bool bg_affects_rasterize() const override { return false; }
    const fx_gamma_ramp* gamma_ramp() const override { return identity_gamma_ramp(); }

private:
    pango_font(g_object_ptr<PangoContext> context,
               pango_description_ptr description,
               cairo_font_options_ptr font_options,
               g_object_ptr<PangoFontset> fontset,
               cairo_context_ptr measurement_context,
               float requested_size,
               uint32_t attrs)
        : context_(std::move(context)),
          description_(std::move(description)),
          font_options_(std::move(font_options)),
          fontset_(std::move(fontset)),
          measurement_context_(std::move(measurement_context)),
          requested_size_(requested_size),
          attrs_(attrs) {}

    uint32_t register_face(PangoFont* face);
    PangoFont* face(uint32_t glyph) const;

    g_object_ptr<PangoContext> context_;
    pango_description_ptr description_;
    cairo_font_options_ptr font_options_;
    // Sublime retains the fontset even though PangoLayout performs fallback independently. Doing
    // the same both validates the description and keeps Pango's resolved font resources alive.
    g_object_ptr<PangoFontset> fontset_;
    cairo_context_ptr measurement_context_;
    float requested_size_ = 0.0f;
    uint32_t attrs_ = 0;
    mutable bool metrics_valid_ = false;
    mutable fx_font_metrics metrics_;
    int32_t space_glyph_ = -1;
    std::vector<g_object_ptr<PangoFont>> faces_;
};

uint32_t pango_font::register_face(PangoFont* face) {
    for (size_t i = 0; i < faces_.size(); ++i) {
        // This is deliberately pointer identity. It is the comparison used by Sublime's Linux
        // backend, unlike its Core Text backend where semantically equal CTFont objects compare
        // equal despite having different addresses.
        if (faces_[i].get() == face) return static_cast<uint32_t>(i);
    }
    if (faces_.size() >= std::numeric_limits<uint16_t>::max()) return 0;
    g_object_ref(face);
    faces_.emplace_back(face);
    return static_cast<uint32_t>(faces_.size() - 1);
}

PangoFont* pango_font::face(uint32_t glyph) const {
    const uint32_t index = glyph >> 16;
    return index < faces_.size() ? faces_[index].get() : nullptr;
}

std::unique_ptr<pango_font> pango_font::create(std::string family, float size, uint32_t attrs) {
    if (!(size > 0.0f) || !std::isfinite(size)) return nullptr;
    if (family == "system") family = "Sans";

    pango_description_ptr description(pango_font_description_new());
    if (!description) return nullptr;
    pango_font_description_set_family(description.get(), family.c_str());

    // Pango sizes are points at 96 DPI, whereas Sublime's cross-platform size is in logical
    // pixels. The Linux binary performs these two float multiplies and truncates to an integer.
    constexpr float kPixelsToPoints = 72.0f / 96.0f;
    const float pango_size = size * kPixelsToPoints * static_cast<float>(PANGO_SCALE);
    if (pango_size > static_cast<float>(std::numeric_limits<int>::max())) return nullptr;
    pango_font_description_set_size(description.get(), static_cast<int>(pango_size));
    if (attrs & FX_FONT_ITALIC) {
        pango_font_description_set_style(description.get(), PANGO_STYLE_ITALIC);
    }
    if (attrs & FX_FONT_BOLD) {
        pango_font_description_set_weight(description.get(), PANGO_WEIGHT_BOLD);
    }

    cairo_font_options_ptr font_options(cairo_font_options_create());
    if (!font_options || cairo_font_options_status(font_options.get()) != CAIRO_STATUS_SUCCESS) {
        return nullptr;
    }
    cairo_antialias_t antialias = CAIRO_ANTIALIAS_SUBPIXEL;
    if (attrs & FX_FONT_NO_ANTIALIAS) {
        antialias = CAIRO_ANTIALIAS_NONE;
    } else if (attrs & FX_FONT_GRAY_ANTIALIAS) {
        antialias = CAIRO_ANTIALIAS_GRAY;
    }
    cairo_font_options_set_antialias(font_options.get(), antialias);

    PangoFontMap* map = pango_cairo_font_map_get_default();
    if (!map) return nullptr;
    g_object_ptr<PangoContext> context(pango_font_map_create_context(map));
    if (!context) return nullptr;
    pango_context_set_font_description(context.get(), description.get());
    PangoLanguage* language = pango_language_get_default();
    pango_context_set_language(context.get(), language);
    g_object_ptr<PangoFontset> fontset(
        pango_context_load_fontset(context.get(), description.get(), language));
    if (!fontset) return nullptr;

    cairo_surface_ptr surface(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 0, 0));
    if (!surface || cairo_surface_status(surface.get()) != CAIRO_STATUS_SUCCESS) return nullptr;
    cairo_context_ptr measurement_context(cairo_create(surface.get()));
    if (!measurement_context || cairo_status(measurement_context.get()) != CAIRO_STATUS_SUCCESS) {
        return nullptr;
    }

    return std::unique_ptr<pango_font>(
        new pango_font(std::move(context), std::move(description), std::move(font_options),
                       std::move(fontset), std::move(measurement_context), size, attrs));
}

fx_font_metrics pango_font::metrics() const {
    if (metrics_valid_) return metrics_;

    PangoFontMap* map = pango_cairo_font_map_get_default();
    g_object_ptr<PangoFont> primary(
        map ? pango_font_map_load_font(map, context_.get(), description_.get()) : nullptr);
    pango_metrics_ptr native_metrics(
        primary ? pango_font_get_metrics(primary.get(), pango_language_get_default()) : nullptr);
    if (native_metrics) {
        // PANGO_PIXELS is exactly the binary's `(value + 512) >> 10`, including its behavior for
        // negative values. Ascender and descender are normally non-negative here.
        metrics_.ascent =
            static_cast<float>(PANGO_PIXELS(pango_font_metrics_get_ascent(native_metrics.get())));
        metrics_.descent =
            static_cast<float>(PANGO_PIXELS(pango_font_metrics_get_descent(native_metrics.get())));
        metrics_.leading = 0.0f;
        metrics_.line_height = metrics_.ascent + metrics_.descent;
    }
    metrics_valid_ = true;
    return metrics_;
}

std::unique_ptr<fx_layout> pango_font::shape(std::string_view utf8) {
    g_object_ptr<PangoLayout> layout(pango_layout_new(context_.get()));
    if (!layout) return nullptr;

    const std::string features = font_features(attrs_);
    PangoAttribute* feature_attribute = pango_attr_font_features_new(features.c_str());
    pango_attribute_list_ptr attributes(pango_attr_list_new());
    if (feature_attribute && attributes) {
        pango_attr_list_insert(attributes.get(), feature_attribute);
        pango_layout_set_attributes(layout.get(), attributes.get());
    } else if (feature_attribute) {
        pango_attribute_destroy(feature_attribute);
    }

    if (utf8.size() > static_cast<size_t>(std::numeric_limits<int>::max())) return nullptr;
    pango_layout_set_text(layout.get(), utf8.data(), static_cast<int>(utf8.size()));

    auto shaped = std::make_unique<fx_layout>();
    float pen = 0.0f;
    pango_iterator_ptr iterator(pango_layout_get_iter(layout.get()));
    if (iterator) {
        do {
            PangoLayoutRun* run = pango_layout_iter_get_run_readonly(iterator.get());
            if (!run || !run->item || !run->glyphs || !run->item->analysis.font) continue;

            const uint32_t face_index = register_face(run->item->analysis.font);
            PangoGlyphString* glyph_string = run->glyphs;
            for (int i = 0; i < glyph_string->num_glyphs; ++i) {
                const PangoGlyphInfo& info = glyph_string->glyphs[i];
                const float advance = static_cast<float>(PANGO_PIXELS(info.geometry.width));
                // Pango's unknown-glyph flag occupies bits above the 16-bit glyph index. Sublime
                // omits those entries because the upper half is reserved for our fallback face.
                if (info.glyph <= std::numeric_limits<uint16_t>::max()) {
                    shaped->glyphs.push_back({
                        .id = (face_index << 16) | static_cast<uint32_t>(info.glyph),
                        .x_offset = pen + static_cast<float>(PANGO_PIXELS(info.geometry.x_offset)),
                        .y_offset = static_cast<float>(PANGO_PIXELS(info.geometry.y_offset)),
                        // The repository-wide fx contract uses UTF-8 byte offsets. Sublime's
                        // Linux-only class converts this to a code-point index before returning to
                        // its own caller; grapheme_shaper performs that boundary conversion here.
                        .cluster = static_cast<uint32_t>(run->item->offset +
                                                         glyph_string->log_clusters[i]),
                    });
                }
                pen += advance;
            }
        } while (pango_layout_iter_next_run(iterator.get()));
    }

    const fx_font_metrics font_metrics = metrics();
    shaped->advance = pen;
    shaped->line_height = font_metrics.ascent + font_metrics.descent;
    return shaped;
}

void pango_font::extents(uint32_t glyph, float scale, vec2& origin, vec2& size) {
    PangoFont* native_face = face(glyph);
    if (!native_face || !measurement_context_ || !(scale > 0.0f) || !std::isfinite(scale)) {
        origin = {};
        size = {};
        return;
    }
    cairo_scaled_font_t* scaled_font =
        pango_cairo_font_get_scaled_font(PANGO_CAIRO_FONT(native_face));
    if (!scaled_font) {
        origin = {};
        size = {};
        return;
    }
    cairo_set_scaled_font(measurement_context_.get(), scaled_font);
    const cairo_glyph_t cairo_glyph = {static_cast<unsigned long>(static_cast<uint16_t>(glyph)),
                                       0.0, 0.0};
    cairo_text_extents_t bounds{};
    cairo_glyph_extents(measurement_context_.get(), &cairo_glyph, 1, &bounds);

    const double native_scale = static_cast<double>(scale);
    origin = {
        std::round(-bounds.x_bearing * native_scale),
        // Cairo reports a negative y bearing for ink above the baseline. Keep the scratch origin
        // baseline-relative here so every fx backend produces bearings with the same meaning.
        std::round(-bounds.y_bearing * native_scale),
    };
    size = {bounds.width * native_scale, bounds.height * native_scale};

    // Sublime adds no border at 1x. At every other scale it pads each side by 2*ceil(scale),
    // large enough for Cairo's scaled/filtering footprint.
    if (scale != 1.0f) {
        const double padding = static_cast<double>(std::ceil(scale));
        origin.x += 2.0 * padding;
        origin.y += 2.0 * padding;
        size.x += 4.0 * padding;
        size.y += 4.0 * padding;
    }
}

void pango_font::rasterize(uint32_t glyph,
                           vec2 position,
                           float scale,
                           fx_pixel_buffer* buffer,
                           color foreground,
                           uint32_t subpixel_order) {
    DCHECK(buffer);
    DCHECK(buffer->pixels);
    DCHECK(buffer->width > 0);
    DCHECK(buffer->height > 0);
    DCHECK(buffer->row_pixels >= buffer->width);
    PangoFont* native_face = face(glyph);
    if (!native_face || !(scale > 0.0f) || !std::isfinite(scale)) {
        return;
    }

    cairo_surface_ptr surface(cairo_image_surface_create_for_data(
        reinterpret_cast<unsigned char*>(buffer->pixels), CAIRO_FORMAT_ARGB32, buffer->width,
        buffer->height, buffer->row_pixels * static_cast<int>(sizeof(uint32_t))));
    if (!surface || cairo_surface_status(surface.get()) != CAIRO_STATUS_SUCCESS) return;
    cairo_context_ptr context(cairo_create(surface.get()));
    if (!context || cairo_status(context.get()) != CAIRO_STATUS_SUCCESS) return;

    cairo_set_source_rgb(context.get(), static_cast<double>(foreground.red()),
                         static_cast<double>(foreground.green()),
                         static_cast<double>(foreground.blue()));
    cairo_scaled_font_t* scaled_font =
        pango_cairo_font_get_scaled_font(PANGO_CAIRO_FONT(native_face));
    if (!scaled_font) return;
    cairo_set_scaled_font(context.get(), scaled_font);

    // ST calls cairo_font_options_set_subpixel_order immediately before attaching this options
    // object to the scratch context. The value comes from GDK's screen font options; keeping it in
    // the glyph-cache key prevents a display/order change from reusing pixels rasterized for a
    // different physical stripe order.
    cairo_font_options_set_subpixel_order(font_options_.get(),
                                          static_cast<cairo_subpixel_order_t>(subpixel_order));
    cairo_set_font_options(context.get(), font_options_.get());
    cairo_scale(context.get(), static_cast<double>(scale), static_cast<double>(scale));

    const double x = position.x / static_cast<double>(scale);
    const double y = position.y / static_cast<double>(scale);
    cairo_glyph_t glyphs[3] = {
        {static_cast<unsigned long>(static_cast<uint16_t>(glyph)), x, y},
        {},
        {},
    };
    int glyph_count = 1;
    if (static_cast<float>(buffer->width) >= requested_size_) {
        if (space_glyph_ < 0) {
            const std::unique_ptr<fx_layout> space = shape(" ");
            space_glyph_ = space && !space->glyphs.empty()
                               ? static_cast<int32_t>(space->glyphs.front().id & 0xffff)
                               : 0;
        }
        glyphs[1] = {
            static_cast<unsigned long>(space_glyph_),
            (position.x - static_cast<double>(buffer->width)) / static_cast<double>(scale), y};
        glyphs[2] = {
            static_cast<unsigned long>(space_glyph_),
            (position.x + static_cast<double>(buffer->width)) / static_cast<double>(scale), y};
        glyph_count = 3;
    }
    cairo_show_glyphs(context.get(), glyphs, glyph_count);
    cairo_surface_flush(surface.get());
}

bool pango_font::is_color_glyph(uint32_t glyph) {
    PangoFont* native_face = face(glyph);
    if (!native_face) return false;
    cairo_scaled_font_t* scaled_font =
        pango_cairo_font_get_scaled_font(PANGO_CAIRO_FONT(native_face));
    if (!scaled_font) return false;
    FT_Face freetype_face = cairo_ft_scaled_font_lock_face(scaled_font);
    if (!freetype_face) return false;

    bool color = false;
    if (FT_HAS_COLOR(freetype_face)) {
        const FT_Error error = FT_Load_Glyph(freetype_face, static_cast<FT_UInt>(glyph & 0xffff),
                                             FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP);
        color = error != FT_Err_Ok || !freetype_face->glyph ||
                freetype_face->glyph->format != FT_GLYPH_FORMAT_OUTLINE;
    }
    cairo_ft_scaled_font_unlock_face(scaled_font);
    return color;
}

}  // namespace

std::unique_ptr<fx_font> fx_create_font(std::string_view family, float size, uint32_t attrs) {
    return pango_font::create(std::string(family), size, attrs);
}
