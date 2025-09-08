#include "base/rand_util.h"
#include "font/font_rasterizer.h"
#include <gtest/gtest.h>

namespace font {

// We should move the rasterized bitmap data *directly* into OpenGL. Manually creating the buffer
// via rearranging pixels will be too slow.
TEST(FontRasterizerTest, RasterizePerformance) {
    auto& rasterizer = FontRasterizer::instance();
    size_t font_id = rasterizer.add_system_font(32);

    auto layout = rasterizer.layout_line(font_id, "a");
    uint32_t glyph_id = layout.glyphs[0].glyph_id;

    for (int i = 0; i < 10000; ++i) {
        auto rglyph = rasterizer.rasterize(font_id, glyph_id);
        EXPECT_GT(rglyph.width, 0);
        EXPECT_GT(rglyph.height, 0);
    }
}

TEST(FontRasterizerTest, LineLayoutPerformance) {
    auto& rasterizer = FontRasterizer::instance();
    size_t font_id = rasterizer.add_system_font(32);

    for (int i = 0; i < 1000; ++i) {
        std::string str = base::rand_bytes_as_string(100);
        auto layout = rasterizer.layout_line(font_id, str);
    }
}

}  // namespace font
