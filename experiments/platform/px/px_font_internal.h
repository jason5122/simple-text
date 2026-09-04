#pragma once

#include "fx/fx.h"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>

struct px_font_t {
    px_font_t(std::string family,
              float requested_size,
              uint32_t attributes,
              std::unique_ptr<fx_font> font)
        : family(std::move(family)),
          requested_size(requested_size),
          attributes(attributes),
          font(std::move(font)) {
        glyph_caches.emplace(100u, std::make_unique<fx_glyph_cache>(this->font.get(), 1.0f));
        glyph_caches.emplace(200u, std::make_unique<fx_glyph_cache>(this->font.get(), 2.0f));
    }

    fx_glyph_cache& glyph_cache(float scale);

    std::string family;
    float requested_size = 0.0f;
    uint32_t attributes = 0;
    std::unique_ptr<fx_font> font;
    std::map<uint32_t, std::unique_ptr<fx_glyph_cache>> glyph_caches;
};
