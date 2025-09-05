#include "base/hash/hash.h"
#include "build/build_config.h"
#include "font/font_rasterizer.h"

namespace font {

FontRasterizer& FontRasterizer::instance() {
    static FontRasterizer renderer;
    return renderer;
}

const Metrics& FontRasterizer::metrics(size_t font_id) const {
    return font_id_to_metrics.at(font_id);
}

std::string_view FontRasterizer::postscript_name(size_t font_id) const {
    return font_id_to_postscript_name.at(font_id);
}

size_t FontRasterizer::hash_font(std::string_view font_name8, int font_size) const {
    uint64_t str_hash = base::hash_string(font_name8);
    return base::hash_combine(str_hash, font_size);
}

#if BUILDFLAG(IS_WIN)
size_t FontRasterizer::hash_font(std::wstring_view font_name16, int font_size) const {
    uint64_t str_hash = base::hash_string(font_name16);
    return base::hash_combine(str_hash, font_size);
}
#endif

}  // namespace font
