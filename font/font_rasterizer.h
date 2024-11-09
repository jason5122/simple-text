#pragma once

#include "build/build_config.h"
#include "font/types.h"
#include <string_view>
#include <unordered_map>
#include <vector>

#if BUILDFLAG(IS_MAC)
#include "base/apple/scoped_cftyperef.h"
#include <CoreText/CoreText.h>
#endif

namespace font {

#if BUILDFLAG(IS_MAC)
using NativeFontType = base::apple::ScopedCFTypeRef<CTFontRef>;
#endif

class FontRasterizer {
public:
    static FontRasterizer& instance();

    size_t addFont(std::string_view font_name8, int font_size, FontStyle style = FontStyle::kNone);
    size_t addSystemFont(int font_size, FontStyle style = FontStyle::kNone);
    const Metrics& metrics(size_t font_id) const;

    RasterizedGlyph rasterize(size_t font_id, uint32_t glyph_id) const;
    LineLayout layoutLine(size_t font_id, std::string_view str8);

private:
    FontRasterizer();
    ~FontRasterizer();

    std::unordered_map<std::string, size_t> font_postscript_name_to_id;
    std::vector<NativeFontType> font_id_to_native;
    std::vector<Metrics> font_id_to_metrics;
    size_t cacheFont(NativeFontType ct_font);
};

}  // namespace font
