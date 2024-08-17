#include "font_rasterizer.h"

namespace font {

FontRasterizer& FontRasterizer::instance() {
    static FontRasterizer renderer;
    return renderer;
}

}
