#pragma once

#include "experiments/platform/fx/fx.h"

#include <map>
#include <memory>

// Opaque to px clients. ST's px_font_t similarly bridges the flat platform API to one fx_font and
// owns renderer/raster-scale caches around it.
struct px_font_t {
    px_font_t(std::unique_ptr<fx_font> value, float point_size)
        : font(std::move(value)), size(point_size) {}

    fx_glyph_cache& glyph_cache(float scale);

    std::unique_ptr<fx_font> font;
    float size = 0.0f;
    std::map<int, std::unique_ptr<fx_glyph_cache>> glyph_caches;
};

// TODO: Move this out of px. Token/trailing-whitespace batching belongs in the editor's
// TextBuffer layout layer (ST's TokenWordWrapper/TokenRenderer side); grapheme shaping belongs
// in the shared shaper (ST's standalone grapheme_shaper). px should only bridge fonts and draw
// whole or already-shaped text.
//
// Reconstructs TextBuffer's token and grapheme shaping policy. This is deliberately separate from
// px_render_context::draw_text(), which is Sublime's whole-string UI text primitive.
std::vector<fx_layout_batch> shape_text_buffer_batches(px_font_t* font, std::string_view utf8);
