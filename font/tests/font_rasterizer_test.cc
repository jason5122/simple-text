#include "base/numeric/literals.h"
#include "build/build_config.h"
#include "font/font_rasterizer.h"
#include <gtest/gtest.h>

// TODO: Remove this.
#include <CoreText/CoreText.h>

namespace {

#if BUILDFLAG(IS_MAC)
const std::string kOSFontFace = "Menlo";
#elif BUILDFLAG(IS_LINUX)
const std::string kOSFontFace = "Monospace";
#elif BUILDFLAG(IS_WIN)
const std::string kOSFontFace = "Consolas";
#endif

}  // namespace

namespace font {

static_assert(!std::is_copy_constructible_v<FontRasterizer>);
static_assert(!std::is_copy_assignable_v<FontRasterizer>);
static_assert(!std::is_move_constructible_v<FontRasterizer>);
static_assert(!std::is_move_assignable_v<FontRasterizer>);

static_assert(std::is_trivially_copy_constructible_v<LineLayout::ConstIterator>);
static_assert(std::is_trivially_copy_assignable_v<LineLayout::ConstIterator>);

TEST(FontRasterizerTest, LayoutLine1) {
    auto& rasterizer = FontRasterizer::instance();
    size_t font_id = rasterizer.addFont(kOSFontFace, 32);

    const std::string line = "Hello😄🙂hi";
    auto layout = rasterizer.layoutLine(font_id, line);

    EXPECT_GT(layout.width, 0);
    EXPECT_EQ(layout.runs.size(), 3_Z);

    const auto& emoji_glyphs = layout.runs[1].glyphs;
    EXPECT_EQ(emoji_glyphs.size(), 2_Z);
    EXPECT_EQ(emoji_glyphs[0].index, 5_Z);
    EXPECT_EQ(emoji_glyphs[1].index, 9_Z);

    const auto& hi_glyphs = layout.runs[2].glyphs;
    EXPECT_EQ(hi_glyphs.size(), 2_Z);
    EXPECT_EQ(hi_glyphs[0].index, 13_Z);
    EXPECT_EQ(hi_glyphs[1].index, 14_Z);

    // Emojis should be colored and should have 4 channels.
    size_t emoji_font_id = layout.runs[1].font_id;
    for (const auto& glyph : emoji_glyphs) {
        auto rglyph = rasterizer.rasterize(emoji_font_id, glyph.glyph_id);
        EXPECT_TRUE(rglyph.colored);
        EXPECT_EQ(rglyph.buffer.size(), rglyph.width * rglyph.height * 4_Z);
    }

    // Regular text should not be colored and should have 3 channels.
    size_t monospace_font_id = layout.runs[2].font_id;
    for (const auto& glyph : hi_glyphs) {
        auto rglyph = rasterizer.rasterize(monospace_font_id, glyph.glyph_id);
        EXPECT_FALSE(rglyph.colored);
        EXPECT_EQ(rglyph.buffer.size(), rglyph.width * rglyph.height * 3_Z);
    }

    // Ensure positions are monotonically increasing.
    int prev_x = 0;
    for (const auto& run : layout.runs) {
        for (const auto& glyph : run.glyphs) {
            EXPECT_GE(glyph.position.x, prev_x);
            prev_x = std::max(glyph.position.x, prev_x);
        }
    }
}

TEST(FontRasterizerTest, LayoutLine2) {
    auto& rasterizer = FontRasterizer::instance();
    size_t font_id = rasterizer.addFont(kOSFontFace, 32);

    const std::string line = "Hello😄🙂hi";
    auto layout = rasterizer.layoutLine(font_id, line);

    int total_advance = 0;
    for (const auto& run : layout.runs) {
        for (const auto& glyph : run.glyphs) {
            total_advance += glyph.advance.x;
        }
    }

    EXPECT_EQ(total_advance, layout.width);
}

namespace {
void ExtractPixels(CGContextRef context) {
    uint8_t* bitmap_data = static_cast<uint8_t*>(CGBitmapContextGetData(context));
    // size_t height = CGBitmapContextGetHeight(context);
    // size_t bytes_per_row = CGBitmapContextGetBytesPerRow(context);
    // size_t len = height * bytes_per_row;

    // size_t pixels = len / 4;
    // std::vector<uint8_t> buffer;
    // size_t size = pixels * 3;

    // buffer.reserve(size);
    // for (size_t i = 0; i < pixels; ++i) {
    //     size_t offset = i * 4;
    //     buffer.emplace_back(bitmap_data[offset + 2]);
    //     buffer.emplace_back(bitmap_data[offset + 1]);
    //     buffer.emplace_back(bitmap_data[offset]);
    // }

    auto buffer = std::make_unique<uint8_t*>(bitmap_data);
}
}  // namespace

TEST(FontRasterizerTest, RasterizePerformance) {
    auto& rasterizer = FontRasterizer::instance();
    size_t font_id = rasterizer.addFont(kOSFontFace, 32);

    auto layout = rasterizer.layoutLine(font_id, "a");
    uint32_t glyph_id = layout.runs[0].glyphs[0].glyph_id;

    // for (size_t i = 0; i < 10000; ++i) {
    //     auto rglyph = rasterizer.rasterize(font_id, glyph_id);
    //     EXPECT_GT(rglyph.width, 0);
    //     EXPECT_GT(rglyph.height, 0);
    // }

    CTFontRef ct_font = CTFontCreateUIFontForLanguage(kCTFontUIFontSystem, 32, nullptr);
    for (size_t i = 0; i < 10000; i++) {
        CGGlyph glyph_index = glyph_id;

        CGRect bounds;
        CTFontGetBoundingRectsForGlyphs(ct_font, kCTFontOrientationDefault, &glyph_index, &bounds,
                                        1);
        int32_t rasterized_left = std::floor(bounds.origin.x);
        uint32_t rasterized_width =
            std::ceil(bounds.origin.x - rasterized_left + bounds.size.width);
        int32_t rasterized_descent = std::ceil(-bounds.origin.y);
        int32_t rasterized_ascent = std::ceil(bounds.size.height + bounds.origin.y);
        uint32_t rasterized_height = rasterized_descent + rasterized_ascent;

        CGColorSpaceRef color_space = CGColorSpaceCreateDeviceRGB();
        CGContextRef context = CGBitmapContextCreate(
            nullptr, rasterized_width, rasterized_height, 8, rasterized_width * 4, color_space,
            kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host);
        CGPoint rasterization_origin = CGPointZero;
        CTFontDrawGlyphs(ct_font, &glyph_index, &rasterization_origin, 1, context);
        ExtractPixels(context);
    }
}

}  // namespace font
