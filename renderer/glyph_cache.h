#pragma once

#include "font/rasterizer.h"
#include "renderer/atlas.h"
#include "renderer/opengl_types.h"
#include <array>
#include <optional>
#include <string>
#include <unordered_map>

namespace renderer {
struct AtlasGlyph {
    Vec4 glyph;
    Vec4 uv;
    int32_t advance;
    bool colored;
};

class GlyphCache {
public:
    GlyphCache(font::FontRasterizer& font_rasterizer);
    void setup();
    AtlasGlyph& getGlyph(std::string_view key);
    void bindTexture();
    int lineHeight();

private:
    font::FontRasterizer& font_rasterizer;

    Atlas atlas;

    struct string_hash {
        using hash_type = std::hash<std::string_view>;
        using is_transparent = void;

        size_t operator()(const char* str) const {
            return hash_type{}(str);
        }
        size_t operator()(std::string_view str) const {
            return hash_type{}(str);
        }
        size_t operator()(std::string const& str) const {
            return hash_type{}(str);
        }
    };

    // using cache_type = std::unordered_map<std::string, AtlasGlyph>;
    using cache_type = std::unordered_map<std::string, AtlasGlyph, string_hash, std::equal_to<>>;

    static constexpr size_t ascii_size = 0x7e - 0x20 + 1;
    using ascii_cache_type = std::array<std::optional<AtlasGlyph>, ascii_size>;

    cache_type cache;
    ascii_cache_type ascii_cache;

    AtlasGlyph createGlyph(std::string_view str8);
};
}
