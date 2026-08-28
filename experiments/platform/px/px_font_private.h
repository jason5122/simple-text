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

std::vector<fx_layout_batch> px_shape_text(px_font_t* font, std::string_view utf8);
