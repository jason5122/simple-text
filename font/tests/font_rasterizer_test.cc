#include <gtest/gtest.h>

#include "base/numeric/literals.h"
#include "build/build_config.h"
#include "font/font_rasterizer.h"
#include "util/random_util.h"

namespace font {

static_assert(!std::is_copy_constructible_v<FontRasterizer>);
static_assert(!std::is_copy_assignable_v<FontRasterizer>);
static_assert(!std::is_move_constructible_v<FontRasterizer>);
static_assert(!std::is_move_assignable_v<FontRasterizer>);

TEST(FontRasterizerTest, LayoutLine1) {
    auto& rasterizer = FontRasterizer::instance();
    size_t font_id = rasterizer.addSystemFont(32);
    auto layout = rasterizer.layoutLine(font_id, "Hello😄🙂hi");

    EXPECT_GT(layout.width, 0);
    EXPECT_EQ(layout.glyphs.size(), 9_Z);

    auto& letter_glyph_1 = layout.glyphs[0];
    auto& letter_glyph_2 = layout.glyphs[1];
    auto& emoji_glyph_1 = layout.glyphs[5];
    auto& emoji_glyph_2 = layout.glyphs[6];

    EXPECT_EQ(letter_glyph_1.index, 0_Z);
    EXPECT_EQ(letter_glyph_2.index, 1_Z);
    EXPECT_EQ(emoji_glyph_1.index, 5_Z);
    EXPECT_EQ(emoji_glyph_2.index, 9_Z);

    // Emojis should be colored and should have 4 channels.
    auto emoji_rglyph = rasterizer.rasterize(emoji_glyph_1.font_id, emoji_glyph_1.glyph_id);
    EXPECT_TRUE(emoji_rglyph.colored);
    EXPECT_EQ(emoji_rglyph.buffer.size(), emoji_rglyph.width * emoji_rglyph.height * 4_Z);

    // Regular text should not be colored, but should also have 4 channels.
    auto letter_rglyph = rasterizer.rasterize(letter_glyph_1.font_id, letter_glyph_1.glyph_id);
    EXPECT_FALSE(letter_rglyph.colored);
    EXPECT_EQ(letter_rglyph.buffer.size(), letter_rglyph.width * letter_rglyph.height * 4_Z);

    // Ensure positions are monotonically increasing.
    int prev_x = 0;
    for (const auto& glyph : layout.glyphs) {
        EXPECT_GE(glyph.position.x, prev_x);
        prev_x = std::max(glyph.position.x, prev_x);
    }
}

TEST(FontRasterizerTest, LayoutLine2) {
    auto& rasterizer = FontRasterizer::instance();
    size_t font_id = rasterizer.addSystemFont(32);
    auto layout = rasterizer.layoutLine(font_id, "Hello😄🙂hi");

    int total_advance = 0;
    for (const auto& glyph : layout.glyphs) {
        total_advance += glyph.advance.x;
    }

    EXPECT_EQ(total_advance, layout.width);
}

// We should move the rasterized bitmap data *directly* into OpenGL. Manually creating the buffer
// via rearranging pixels will be too slow.
TEST(FontRasterizerTest, RasterizePerformance) {
    auto& rasterizer = FontRasterizer::instance();
    size_t font_id = rasterizer.addSystemFont(32);

    auto layout = rasterizer.layoutLine(font_id, "a");
    uint32_t glyph_id = layout.glyphs[0].glyph_id;

    for (int i = 0; i < 10000; ++i) {
        auto rglyph = rasterizer.rasterize(font_id, glyph_id);
        EXPECT_GT(rglyph.width, 0);
        EXPECT_GT(rglyph.height, 0);
    }
}

TEST(FontRasterizerTest, LineLayoutPerformance) {
    auto& rasterizer = FontRasterizer::instance();
    size_t font_id = rasterizer.addSystemFont(32);

    for (int i = 0; i < 1000; ++i) {
        std::string str = util::RandomString(100);
        auto layout = rasterizer.layoutLine(font_id, str);
    }
}

}  // namespace font
