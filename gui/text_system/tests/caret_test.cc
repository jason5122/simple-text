#include <gtest/gtest.h>

#include "base/numeric/literals.h"
#include "build/build_config.h"
#include "font/font_rasterizer.h"
#include "gui/text_system/caret.h"
#include "util/std_print.h"

namespace gui {

namespace {
font::LineLayout CreateLayout(std::string_view str) {
    auto& rasterizer = font::FontRasterizer::instance();
    size_t font_id = rasterizer.addSystemFont(32);
    return rasterizer.layoutLine(font_id, str);
}
}  // namespace

TEST(CaretTest, ColumnAtX) {
    auto layout = CreateLayout("Hello😄🙂hi");
    size_t prev_index = 0;
    for (int x = 0; x < layout.width; ++x) {
        size_t index = Caret::columnAtX(layout, x);
        EXPECT_GE(index, prev_index);
        prev_index = index;
    }

    EXPECT_EQ(Caret::columnAtX(layout, 99999), layout.length);
    EXPECT_EQ(Caret::columnAtX(layout, 0), 0_Z);
}

TEST(CaretTest, XAtColumn) {
    auto layout = CreateLayout("Hello😄🙂hi");
    int prev_x = 0;
    for (size_t index = 0; index < layout.length; ++index) {
        int x = Caret::xAtColumn(layout, index);
        EXPECT_GE(x, prev_x);
        prev_x = x;
    }

    EXPECT_EQ(Caret::xAtColumn(layout, 99999), layout.width);
    EXPECT_EQ(Caret::xAtColumn(layout, 0), 0);
}

// Ensure moving while at the beginning/end does nothing.
TEST(CaretTest, PreventMovingByGlyphAtBeginningOrEnd) {
    auto layout1 = CreateLayout("Hello😄🙂hi");
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(Caret::moveToPrevGlyph(layout1, 0), 0_Z);
        EXPECT_EQ(Caret::moveToNextGlyph(layout1, layout1.length), 0_Z);
    }
    auto layout2 = CreateLayout("");
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(Caret::moveToPrevGlyph(layout2, 0), 0_Z);
        EXPECT_EQ(Caret::moveToNextGlyph(layout2, 0), 0_Z);
    }
}

// Ensure moving while at the beginning/end does nothing.
TEST(CaretTest, PreventMovingByWordAtBeginningOrEnd) {
    base::PieceTree tree1{"abc🙂def"};
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(Caret::prevWordStart(tree1, 0), 0_Z);
        EXPECT_EQ(Caret::nextWordEnd(tree1, tree1.length()), tree1.length());
    }
    base::PieceTree tree2{""};
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(Caret::prevWordStart(tree2, 0), 0_Z);
        EXPECT_EQ(Caret::nextWordEnd(tree2, 0), 0_Z);
    }
}

TEST(CaretTest, MoveToPrevGlyph) {
    auto layout = CreateLayout("abc🙂def");
    EXPECT_EQ(Caret::moveToPrevGlyph(layout, layout.length), 1_Z);      // f
    EXPECT_EQ(Caret::moveToPrevGlyph(layout, layout.length - 1), 1_Z);  // e
    EXPECT_EQ(Caret::moveToPrevGlyph(layout, layout.length - 2), 1_Z);  // d
    EXPECT_EQ(Caret::moveToPrevGlyph(layout, layout.length - 3), 4_Z);  // 🙂
    EXPECT_EQ(Caret::moveToPrevGlyph(layout, layout.length - 7), 1_Z);  // c
    EXPECT_EQ(Caret::moveToPrevGlyph(layout, layout.length - 8), 1_Z);  // b
    EXPECT_EQ(Caret::moveToPrevGlyph(layout, layout.length - 9), 1_Z);  // a
    EXPECT_EQ(Caret::moveToPrevGlyph(layout, layout.length - 10), 0_Z);
}

TEST(CaretTest, MoveToNextGlyph) {
    auto layout = CreateLayout("abc🙂def");
    EXPECT_EQ(Caret::moveToNextGlyph(layout, 0), 1_Z);  // a
    EXPECT_EQ(Caret::moveToNextGlyph(layout, 1), 1_Z);  // b
    EXPECT_EQ(Caret::moveToNextGlyph(layout, 2), 1_Z);  // c
    EXPECT_EQ(Caret::moveToNextGlyph(layout, 3), 4_Z);  // 🙂
    EXPECT_EQ(Caret::moveToNextGlyph(layout, 7), 1_Z);  // d
    EXPECT_EQ(Caret::moveToNextGlyph(layout, 8), 1_Z);  // e
    EXPECT_EQ(Caret::moveToNextGlyph(layout, 9), 1_Z);  // f
    EXPECT_EQ(Caret::moveToNextGlyph(layout, 10), 0_Z);
}

TEST(CaretTest, PrevWordStart1) {
    base::PieceTree tree{"abc🙂def"};
    EXPECT_EQ(Caret::prevWordStart(tree, 10), 7_Z);
    EXPECT_EQ(Caret::prevWordStart(tree, 9), 7_Z);
    EXPECT_EQ(Caret::prevWordStart(tree, 8), 7_Z);
    EXPECT_EQ(Caret::prevWordStart(tree, 7), 3_Z);
    EXPECT_EQ(Caret::prevWordStart(tree, 3), 0_Z);
    EXPECT_EQ(Caret::prevWordStart(tree, 2), 0_Z);
    EXPECT_EQ(Caret::prevWordStart(tree, 1), 0_Z);
    EXPECT_EQ(Caret::prevWordStart(tree, 0), 0_Z);
}

TEST(CaretTest, PrevWordStart2) {
    base::PieceTree tree{"🙂🙂🙂"};
    for (size_t i = 0; i <= tree.length(); ++i) {
        EXPECT_EQ(Caret::prevWordStart(tree, i), 0_Z);
    }
}

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

}  // namespace gui
