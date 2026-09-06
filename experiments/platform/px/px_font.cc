#include "experiments/platform/px/px.h"

#include "build/build_config.h"
#include "experiments/platform/px/px_font_internal.h"

#include <bit>
#include <cmath>
#include <map>
#include <memory>
#include <string>
#include <tuple>

namespace {

using font_key = std::tuple<std::string, uint32_t, uint32_t>;

std::map<font_key, std::unique_ptr<px_font_t>>& fonts() {
    // Process lifetime, like ST's font map. Stable px_font_t addresses are also safe renderer
    // cache keys; a destroyed font can never be replaced at the same address.
    static auto* value = new std::map<font_key, std::unique_ptr<px_font_t>>;
    return *value;
}

}  // namespace

fx_glyph_cache& px_font_t::glyph_cache(float scale) {
    const uint32_t key = static_cast<uint32_t>(static_cast<double>(scale) * 100.0);
    auto& cache = glyph_caches[key];
    if (!cache) {
        cache = std::make_unique<fx_glyph_cache>(font.get(), scale);
    }
    return *cache;
}

px_font_t* px_create_font(const char* family, float size, uint32_t attrs) {
    if (!family || size <= 0.0f) {
        return nullptr;
    }
    const font_key key{family, std::bit_cast<uint32_t>(size), attrs};
    auto found = fonts().find(key);
    if (found != fonts().end()) {
        return found->second.get();
    }

    float native_size = size;
#if BUILDFLAG(IS_WIN)
    // DirectWrite consumes DIPs while Sublime's setting is a typographic point size.
    native_size = std::floor(size * 96.0f / 72.0f + 0.5f);
#elif BUILDFLAG(IS_LINUX)
    // The Linux factory passes this exact scale as a separate double before the Pango backend
    // converts native pixels back to points. Do not apply DirectWrite's integer-size rounding.
    native_size = size * 96.0f / 72.0f;
#endif
    std::unique_ptr<fx_font> font = fx_create_font(family, native_size, attrs);
    if (!font) {
        return nullptr;
    }
    auto px_font = std::make_unique<px_font_t>(family, size, attrs, std::move(font));
    px_font_t* result = px_font.get();
    fonts().emplace(key, std::move(px_font));
    return result;
}

float px_font_em_width(px_font_t* font) {
    return font && font->font ? font->font->widths().em_width : 0.0f;
}

bool px_font_is_monospace(px_font_t* font) {
    return font && font->font && font->font->widths().monospace;
}

px_font_metrics px_font_get_metrics(px_font_t* font) {
    if (!font || !font->font) {
        return {};
    }
    const fx_font_metrics metrics = font->font->metrics();
    return {
        .ascent = metrics.ascent,
        .descent = metrics.descent,
        .leading = metrics.leading,
        .line_height = metrics.line_height,
        .raster_ascent = font->font->raster_ascent(),
    };
}

std::unique_ptr<fx_layout> px_shape_text(px_font_t* font, std::string_view utf8) {
    return font && font->font ? font->font->shape(utf8) : nullptr;
}

std::unique_ptr<fx_layout> px_shape_text(px_font_t* font, std::u32string_view utf32) {
    return font && font->font ? font->font->shape(utf32) : nullptr;
}

void px_render_context::draw_text(px_font_t* font,
                                  vec2 position,
                                  color value,
                                  std::string_view utf8,
                                  bool subpixel_positioning) {
    if (utf8.empty()) {
        return;
    }
    std::unique_ptr<fx_layout> layout = px_shape_text(font, utf8);
    if (layout) {
        draw_shaped_text(font, position, value, layout.get(), subpixel_positioning);
    }
}
