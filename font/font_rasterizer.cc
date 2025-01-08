#include "font_rasterizer.h"

#include "base/hash/hash_combine.h"
#include "third_party/hash_maps/rapidhash.h"

namespace font {

FontRasterizer& FontRasterizer::instance() {
    static FontRasterizer renderer;
    return renderer;
}

const Metrics& FontRasterizer::metrics(size_t font_id) const {
    return font_id_to_metrics.at(font_id);
}

std::string_view FontRasterizer::postscriptName(size_t font_id) const {
    return font_id_to_postscript_name.at(font_id);
}

size_t FontRasterizer::hashFont(std::string_view font_name8, int font_size) const {
    uint64_t hash = rapidhash(font_name8.data(), font_name8.length());
    return base::hash_combine(hash, font_size);
}

}  // namespace font
