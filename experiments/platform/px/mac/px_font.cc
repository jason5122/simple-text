#include "experiments/platform/px/px.h"

#include "experiments/platform/px/px_font_private.h"

#include <bit>
#include <iterator>
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
    const int key = static_cast<int>(scale * 100.0f + 0.5f);
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

    std::unique_ptr<fx_font> font = fx_create_font(family, size, attrs);
    if (!font) {
        return nullptr;
    }
    auto px_font = std::make_unique<px_font_t>(std::move(font), size);
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
    return {metrics.ascent, metrics.descent, metrics.leading, metrics.line_height};
}

std::vector<fx_layout_batch> shape_text_buffer_batches(px_font_t* font, std::string_view utf8) {
    if (!font || !font->font || utf8.empty()) {
        return {};
    }
    std::vector<fx_layout_batch> result;
    const auto is_whitespace = [](char c) { return c == ' ' || c == '\t'; };
    double pen_x = 0.0;
    for (size_t start = 0; start < utf8.size();) {
        size_t end = start;
        if (is_whitespace(utf8[end])) {
            while (end < utf8.size() && is_whitespace(utf8[end])) {
                ++end;
            }
        } else {
            while (end < utf8.size() && !is_whitespace(utf8[end])) {
                ++end;
            }
            while (end < utf8.size() && is_whitespace(utf8[end])) {
                ++end;
            }
        }

        // TextBuffer lays out a token together with its trailing whitespace. The token width is
        // accumulated in float by the shaper, then added to the line pen in double precision.
        std::vector<fx_layout_batch> batches = fx_shape_graphemes(
            font->font.get(), utf8.substr(start, end - start), font->size <= 16.0f);
        for (fx_layout_batch& batch : batches) {
            batch.x_offset += pen_x;
        }
        if (!batches.empty()) {
            const fx_layout_batch& last = batches.back();
            pen_x = last.x_offset + static_cast<double>(last.layout.advance);
        }
        result.insert(result.end(), std::make_move_iterator(batches.begin()),
                      std::make_move_iterator(batches.end()));
        start = end;
    }
    return result;
}

void px_render_context::draw_text(px_font_t* font,
                                  vec2 position,
                                  fcolor color,
                                  std::string_view utf8,
                                  bool subpixel_positioning) {
    if (!font || !font->font || utf8.empty()) {
        return;
    }
    std::unique_ptr<fx_layout> layout = font->font->shape(utf8);
    if (layout) {
        draw_shaped_text(font, position, color, layout.get(), subpixel_positioning);
    }
}
