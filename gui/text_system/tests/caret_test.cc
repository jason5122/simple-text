#include "base/numeric/literals.h"
#include "build/build_config.h"
#include "font/font_rasterizer.h"
#include "gui/text_system/caret.h"
#include <gtest/gtest.h>

// namespace {

// #if BUILDFLAG(IS_MAC)
// const std::string kOSFontFace = "Menlo";
// #elif BUILDFLAG(IS_LINUX)
// const std::string kOSFontFace = "Monospace";
// #elif BUILDFLAG(IS_WIN)
// const std::string kOSFontFace = "Consolas";
// #endif

// }

namespace gui {

// TEST(CaretTest, MoveToX) {
//     auto& rasterizer = font::FontRasterizer::instance();
//     size_t font_id = rasterizer.addFont(kOSFontFace, 32);

//     const std::string line = "Hello😄🙂hi";
//     const auto layout = rasterizer.layoutLine(font_id, line);
//     Caret caret;

//     size_t prev_index = 0;
//     // int prev_x = 0;
//     for (int x = 0; x < layout.width; ++x) {
//         caret.moveToX(layout, 0, x);

//         EXPECT_GE(caret.index, prev_index);
//         // EXPECT_GE(caret.x, prev_x);

//         prev_index = caret.index;
//         // prev_x = caret.x;
//     }

//     caret.moveToX(layout, 99999);
//     EXPECT_EQ(caret.index, layout.length);
//     // EXPECT_EQ(caret.x, layout.width);

//     caret.moveToX(layout, 0);
//     EXPECT_EQ(caret.index, 0_Z);
//     // EXPECT_EQ(caret.x, 0);
// }

// TEST(CaretTest, MoveToIndex) {
//     auto& rasterizer = font::FontRasterizer::instance();
//     size_t font_id = rasterizer.addFont(kOSFontFace, 32);

//     const std::string line = "Hello😄🙂hi";
//     const auto layout = rasterizer.layoutLine(font_id, line);
//     Caret caret;

//     size_t prev_index = 0;
//     // int prev_x = 0;
//     for (size_t index = 0; index < line.length(); ++index) {
//         caret.moveToIndex(layout, index);

//         EXPECT_GE(caret.index, prev_index);
//         // EXPECT_GE(caret.x, prev_x);

//         prev_index = caret.index;
//         // prev_x = caret.x;
//     }

//     caret.moveToIndex(layout, 99999);
//     EXPECT_EQ(caret.index, layout.length);
//     // EXPECT_EQ(caret.x, layout.width);

//     caret.moveToIndex(layout, 0);
//     EXPECT_EQ(caret.index, 0_Z);
//     // EXPECT_EQ(caret.x, 0);
// }

// TEST(CaretTest, MoveToPrevGlyph) {
//     auto& rasterizer = font::FontRasterizer::instance();
//     size_t font_id = rasterizer.addFont(kOSFontFace, 32);

//     const std::string line = "Hello😄🙂hi";
//     const auto layout = rasterizer.layoutLine(font_id, line);
//     Caret caret;

//     caret.moveToIndex(layout, 0, 99999);
//     size_t prev_index = caret.col;
//     int prev_x = caret.x;
//     while (caret.col > 0) {
//         caret.moveToPrevGlyph(layout, 0, caret.col);

//         EXPECT_LT(caret.col, prev_index);
//         EXPECT_LT(caret.x, prev_x);

//         prev_index = caret.col;
//         prev_x = caret.x;
//     }

//     // Ensure moving while at the beginning does nothing.
//     for (size_t i = 0; i < 10; i++) {
//         caret.moveToPrevGlyph(layout, 0, 0);
//         EXPECT_EQ(caret.col, 0_Z);
//         EXPECT_EQ(caret.x, 0);
//     }
// }

// TEST(CaretTest, MoveToNextGlyph) {
//     auto& rasterizer = font::FontRasterizer::instance();
//     size_t font_id = rasterizer.addFont(kOSFontFace, 32);

//     const std::string line = "Hello😄🙂hi";
//     const auto layout = rasterizer.layoutLine(font_id, line);
//     Caret caret;

//     caret.moveToIndex(layout, 0, 0);
//     size_t prev_index = caret.col;
//     int prev_x = caret.x;
//     while (caret.col < layout.length) {
//         caret.moveToNextGlyph(layout, 0, caret.col);

//         EXPECT_GT(caret.col, prev_index);
//         EXPECT_GT(caret.x, prev_x);

//         prev_index = caret.col;
//         prev_x = caret.x;
//     }

//     // Ensure moving while at the end does nothing.
//     for (size_t i = 0; i < 10; i++) {
//         caret.moveToNextGlyph(layout, 0, layout.length);
//         EXPECT_EQ(caret.col, layout.length);
//         EXPECT_EQ(caret.x, layout.width);
//     }
// }

// TEST(CaretTest, MoveInEmptyLayout) {
//     auto& rasterizer = font::FontRasterizer::instance();
//     size_t font_id = rasterizer.addFont(kOSFontFace, 32);

//     const std::string line = "";
//     const auto layout = rasterizer.layoutLine(font_id, line);
//     Caret caret;

//     caret.moveToIndex(layout, 0, 0);

//     for (size_t i = 0; i < 10; i++) {
//         caret.moveToPrevGlyph(layout, 0, caret.col);
//         EXPECT_EQ(caret.col, layout.length);
//         EXPECT_EQ(caret.x, layout.width);
//     }
//     for (size_t i = 0; i < 10; i++) {
//         caret.moveToNextGlyph(layout, 0, caret.col);
//         EXPECT_EQ(caret.col, layout.length);
//         EXPECT_EQ(caret.x, layout.width);
//     }
// }

TEST(CaretTest, ComparisonOperators) {
    EXPECT_TRUE(Caret{5} == Caret{5});
    EXPECT_TRUE(Caret{5} == Caret{5});

    EXPECT_TRUE(Caret{0} != Caret{5});
    EXPECT_FALSE(Caret{5} != Caret{5});

    EXPECT_TRUE(Caret{0} < Caret{5});
    EXPECT_FALSE(Caret{5} < Caret{5});

    EXPECT_TRUE(Caret{5} > Caret{0});
    EXPECT_FALSE(Caret{5} > Caret{5});

    EXPECT_TRUE(Caret{0} <= Caret{5});
    EXPECT_TRUE(Caret{5} <= Caret{5});
    EXPECT_FALSE(Caret{5} <= Caret{0});

    EXPECT_TRUE(Caret{5} >= Caret{0});
    EXPECT_TRUE(Caret{5} >= Caret{5});
    EXPECT_FALSE(Caret{0} >= Caret{5});
}

}
