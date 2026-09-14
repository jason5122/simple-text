#include "base/check.h"
#include "build/build_config.h"
#include "gui/renderer/font_cache.h"
#include <cmath>
#include <spdlog/spdlog.h>

namespace gui {

namespace {

constexpr std::string_view kSystemFontFamily = "system";

// Converts a point size to the size fx expects from each backend. Mirrors px_create_font().
float native_font_size(int font_size) {
    const float size = static_cast<float>(font_size);
#if BUILDFLAG(IS_WIN)
    // DirectWrite consumes DIPs while the requested size is a typographic point size.
    return std::floor(size * 96.0f / 72.0f + 0.5f);
#elif BUILDFLAG(IS_LINUX)
    // The Pango backend converts native pixels back to points itself; don't round here.
    return size * 96.0f / 72.0f;
#else
    return size;
#endif
}

}  // namespace

size_t FontCache::add_font(std::string_view family, int font_size, uint32_t attrs) {
    const std::tuple<std::string, int, uint32_t> key{std::string(family), font_size, attrs};
    if (auto it = ids_.find(key); it != ids_.end()) {
        return it->second;
    }

    std::unique_ptr<fx_font> font = fx_create_font(family, native_font_size(font_size), attrs);
    if (!font) {
        spdlog::error("FontCache::add_font() error: could not create font \"{}\" at size {}.",
                      family, font_size);
    }
    CHECK(font);

    // fx reports metrics as positive logical distances from the baseline; round each one up
    // separately so the baseline lands on a device pixel.
    const fx_font_metrics logical = font->metrics();
    const int ascent = static_cast<int>(std::ceil(logical.ascent * kScaleFactor));
    const int descent = static_cast<int>(std::ceil(logical.descent * kScaleFactor));
    const int leading = static_cast<int>(std::ceil(logical.leading * kScaleFactor));
    const Metrics metrics = {
        .line_height = ascent + descent + leading,
        .ascent = ascent,
        .descent = descent,
        .font_size = font_size,
    };

    auto glyph_cache = std::make_unique<fx_glyph_cache>(font.get(), kScaleFactor);

    const size_t font_id = fonts_.size();
    fonts_.push_back({
        .family = std::string(family),
        .font_size = font_size,
        .attrs = attrs,
        .metrics = metrics,
        .font = std::move(font),
        .glyph_cache = std::move(glyph_cache),
    });
    ids_.emplace(key, font_id);
    return font_id;
}

size_t FontCache::add_system_font(int font_size, uint32_t attrs) {
    return add_font(kSystemFontFamily, font_size, attrs);
}

size_t FontCache::resize_font(size_t font_id, int font_size) {
    const Entry& entry = fonts_.at(font_id);
    // Copy the family first: add_font() may grow `fonts_` and invalidate `entry`.
    const std::string family = entry.family;
    const uint32_t attrs = entry.attrs;
    return add_font(family, font_size, attrs);
}

FontCache::Metrics FontCache::metrics(size_t font_id) const { return fonts_.at(font_id).metrics; }

fx_font& FontCache::font(size_t font_id) { return *fonts_.at(font_id).font; }

fx_glyph_cache& FontCache::glyph_cache(size_t font_id) { return *fonts_.at(font_id).glyph_cache; }

}  // namespace gui
