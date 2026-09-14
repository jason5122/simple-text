#pragma once

#include "fx/fx.h"
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace gui {

// Owns every fx_font the UI uses along with a device-scale glyph cache for each. Fonts are
// identified by a stable index that widgets pass around instead of the font itself.
class FontCache {
public:
    // TODO: Don't hard-code the scale factor; take it from the window.
    static constexpr float kScaleFactor = 2.0f;

    // Vertical metrics in device pixels. The alphabetic baseline sits `ascent` below the top of
    // the line box, and `descent` above `ascent + descent`; any leading follows below that.
    struct Metrics {
        int line_height;
        int ascent;
        int descent;
        // The requested size in points, kept so callers can derive a resized font from it.
        int font_size;
    };

    // `attrs` is a combination of FX_FONT_* flags. Returns an existing ID if an identical font has
    // already been added.
    size_t add_font(std::string_view family, int font_size, uint32_t attrs = 0);
    size_t add_system_font(int font_size, uint32_t attrs = 0);
    // Returns the ID of `font_id`'s family and attributes at `font_size`, adding it if needed.
    size_t resize_font(size_t font_id, int font_size);

    Metrics metrics(size_t font_id) const;
    fx_font& font(size_t font_id);
    fx_glyph_cache& glyph_cache(size_t font_id);

private:
    struct Entry {
        std::string family;
        int font_size;
        uint32_t attrs;
        Metrics metrics;
        std::unique_ptr<fx_font> font;
        std::unique_ptr<fx_glyph_cache> glyph_cache;
    };

    std::vector<Entry> fonts_;
    std::map<std::tuple<std::string, int, uint32_t>, size_t> ids_;
};

}  // namespace gui
