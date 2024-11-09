#include "font_rasterizer.h"

namespace font {

FontRasterizer& FontRasterizer::instance() {
    static FontRasterizer renderer;
    return renderer;
}

const Metrics& FontRasterizer::metrics(size_t font_id) const {
    return font_id_to_metrics.at(font_id);
}

}  // namespace font
