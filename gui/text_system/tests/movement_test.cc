#include <gtest/gtest.h>

#include "base/numeric/literals.h"
#include "build/build_config.h"
#include "font/font_rasterizer.h"
#include "gui/text_system/movement.h"

namespace gui {

namespace {
font::LineLayout CreateLayout(std::string_view str) {
    auto& rasterizer = font::FontRasterizer::instance();
    size_t font_id = rasterizer.add_system_font(32);
    return rasterizer.layout_line(font_id, str);
}
}  // namespace

TEST(MovementTest, ColumnAtX) {
    auto layout = CreateLayout("Hello😄🙂hi");
    size_t prev_index = 0;
    for (int x = 0; x < layout.width; ++x) {
        size_t index = movement::column_at_x(layout, x);
        EXPECT_GE(index, prev_index);
        prev_index = index;
    }

    EXPECT_EQ(movement::column_at_x(layout, 99999), layout.length);
    EXPECT_EQ(movement::column_at_x(layout, 0), 0_Z);
}

TEST(MovementTest, XAtColumn) {
    auto layout = CreateLayout("Hello😄🙂hi");
    int prev_x = 0;
    for (size_t index = 0; index < layout.length; ++index) {
        int x = movement::x_at_column(layout, index);
        EXPECT_GE(x, prev_x);
        prev_x = x;
    }

    EXPECT_EQ(movement::x_at_column(layout, 99999), layout.width);
    EXPECT_EQ(movement::x_at_column(layout, 0), 0);
}

// Ensure moving while at the beginning/end does nothing.
TEST(MovementTest, PreventMovingByGlyphAtBeginningOrEnd) {
    auto layout1 = CreateLayout("Hello😄🙂hi");
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(movement::move_to_prev_glyph(layout1, 0), 0_Z);
        EXPECT_EQ(movement::move_to_next_glyph(layout1, layout1.length), 0_Z);
    }
    auto layout2 = CreateLayout("");
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(movement::move_to_prev_glyph(layout2, 0), 0_Z);
        EXPECT_EQ(movement::move_to_next_glyph(layout2, 0), 0_Z);
    }
}

// Ensure moving while at the beginning/end does nothing.
TEST(MovementTest, PreventMovingByWordAtBeginningOrEnd) {
    base::PieceTree tree1{"abc🙂def"};
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(movement::prev_word_start(tree1, 0), 0_Z);
        EXPECT_EQ(movement::next_word_end(tree1, tree1.length()), tree1.length());
    }
    base::PieceTree tree2{""};
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(movement::prev_word_start(tree2, 0), 0_Z);
        EXPECT_EQ(movement::next_word_end(tree2, 0), 0_Z);
    }
}

TEST(MovementTest, MoveToPrevGlyph) {
    auto layout = CreateLayout("abc🙂def");
    EXPECT_EQ(movement::move_to_prev_glyph(layout, layout.length), 1_Z);      // f
    EXPECT_EQ(movement::move_to_prev_glyph(layout, layout.length - 1), 1_Z);  // e
    EXPECT_EQ(movement::move_to_prev_glyph(layout, layout.length - 2), 1_Z);  // d
    EXPECT_EQ(movement::move_to_prev_glyph(layout, layout.length - 3), 4_Z);  // 🙂
    EXPECT_EQ(movement::move_to_prev_glyph(layout, layout.length - 7), 1_Z);  // c
    EXPECT_EQ(movement::move_to_prev_glyph(layout, layout.length - 8), 1_Z);  // b
    EXPECT_EQ(movement::move_to_prev_glyph(layout, layout.length - 9), 1_Z);  // a
    EXPECT_EQ(movement::move_to_prev_glyph(layout, layout.length - 10), 0_Z);
}

TEST(MovementTest, MoveToNextGlyph) {
    auto layout = CreateLayout("abc🙂def");
    EXPECT_EQ(movement::move_to_next_glyph(layout, 0), 1_Z);  // a
    EXPECT_EQ(movement::move_to_next_glyph(layout, 1), 1_Z);  // b
    EXPECT_EQ(movement::move_to_next_glyph(layout, 2), 1_Z);  // c
    EXPECT_EQ(movement::move_to_next_glyph(layout, 3), 4_Z);  // 🙂
    EXPECT_EQ(movement::move_to_next_glyph(layout, 7), 1_Z);  // d
    EXPECT_EQ(movement::move_to_next_glyph(layout, 8), 1_Z);  // e
    EXPECT_EQ(movement::move_to_next_glyph(layout, 9), 1_Z);  // f
    EXPECT_EQ(movement::move_to_next_glyph(layout, 10), 0_Z);
}

TEST(MovementTest, PrevWordStart1) {
    base::PieceTree tree{"abc🙂def"};
    EXPECT_EQ(movement::prev_word_start(tree, 10), 7_Z);
    EXPECT_EQ(movement::prev_word_start(tree, 9), 7_Z);
    EXPECT_EQ(movement::prev_word_start(tree, 8), 7_Z);
    EXPECT_EQ(movement::prev_word_start(tree, 7), 3_Z);
    EXPECT_EQ(movement::prev_word_start(tree, 3), 0_Z);
    EXPECT_EQ(movement::prev_word_start(tree, 2), 0_Z);
    EXPECT_EQ(movement::prev_word_start(tree, 1), 0_Z);
    EXPECT_EQ(movement::prev_word_start(tree, 0), 0_Z);
}

TEST(MovementTest, PrevWordStart2) {
    base::PieceTree tree{"🙂🙂🙂"};
    EXPECT_EQ(movement::prev_word_start(tree, 0), 0_Z);
    EXPECT_EQ(movement::prev_word_start(tree, 4), 0_Z);
    EXPECT_EQ(movement::prev_word_start(tree, 8), 0_Z);
    EXPECT_EQ(movement::prev_word_start(tree, tree.length()), 0_Z);
}

TEST(MovementTest, NextWordEnd1) {
    base::PieceTree tree{"abc🙂def"};
    EXPECT_EQ(movement::next_word_end(tree, 0), 3_Z);
    EXPECT_EQ(movement::next_word_end(tree, 1), 3_Z);
    EXPECT_EQ(movement::next_word_end(tree, 2), 3_Z);
    EXPECT_EQ(movement::next_word_end(tree, 3), 7_Z);
    EXPECT_EQ(movement::next_word_end(tree, 7), 10_Z);
    EXPECT_EQ(movement::next_word_end(tree, 8), 10_Z);
    EXPECT_EQ(movement::next_word_end(tree, 9), 10_Z);
    EXPECT_EQ(movement::next_word_end(tree, 10), 10_Z);
}

TEST(MovementTest, NextWordEnd2) {
    base::PieceTree tree{"🙂🙂🙂"};
    EXPECT_EQ(movement::next_word_end(tree, 0), tree.length());
    EXPECT_EQ(movement::next_word_end(tree, 4), tree.length());
    EXPECT_EQ(movement::next_word_end(tree, 8), tree.length());
    EXPECT_EQ(movement::next_word_end(tree, tree.length()), tree.length());
}

}  // namespace gui
