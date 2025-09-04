#include "editor/movement.h"
#include "font/font_rasterizer.h"
#include <gtest/gtest.h>

namespace editor {

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
        size_t index = column_at_x(layout, x);
        EXPECT_GE(index, prev_index);
        prev_index = index;
    }

    EXPECT_EQ(column_at_x(layout, 99999), layout.length);
    EXPECT_EQ(column_at_x(layout, 0), size_t{0});
}

TEST(MovementTest, XAtColumn) {
    auto layout = CreateLayout("Hello😄🙂hi");
    int prev_x = 0;
    for (size_t index = 0; index < layout.length; ++index) {
        int x = x_at_column(layout, index);
        EXPECT_GE(x, prev_x);
        prev_x = x;
    }

    EXPECT_EQ(x_at_column(layout, 99999), layout.width);
    EXPECT_EQ(x_at_column(layout, 0), 0);
}

// Ensure moving while at the beginning/end does nothing.
TEST(MovementTest, PreventMovingByGlyphAtBeginningOrEnd) {
    auto layout1 = CreateLayout("Hello😄🙂hi");
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(move_to_prev_glyph(layout1, 0), size_t{0});
        EXPECT_EQ(move_to_next_glyph(layout1, layout1.length), size_t{0});
    }
    auto layout2 = CreateLayout("");
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(move_to_prev_glyph(layout2, 0), size_t{0});
        EXPECT_EQ(move_to_next_glyph(layout2, 0), size_t{0});
    }
}

// Ensure moving while at the beginning/end does nothing.
TEST(MovementTest, PreventMovingByWordAtBeginningOrEnd) {
    PieceTree tree1{"abc🙂def"};
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(prev_word_start(tree1, 0), size_t{0});
        EXPECT_EQ(next_word_end(tree1, tree1.length()), tree1.length());
    }
    PieceTree tree2{""};
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(prev_word_start(tree2, 0), size_t{0});
        EXPECT_EQ(next_word_end(tree2, 0), size_t{0});
    }
}

TEST(MovementTest, MoveToPrevGlyph) {
    auto layout = CreateLayout("abc🙂def");
    EXPECT_EQ(move_to_prev_glyph(layout, layout.length), size_t{1});      // f
    EXPECT_EQ(move_to_prev_glyph(layout, layout.length - 1), size_t{1});  // e
    EXPECT_EQ(move_to_prev_glyph(layout, layout.length - 2), size_t{1});  // d
    EXPECT_EQ(move_to_prev_glyph(layout, layout.length - 3), size_t{4});  // 🙂
    EXPECT_EQ(move_to_prev_glyph(layout, layout.length - 7), size_t{1});  // c
    EXPECT_EQ(move_to_prev_glyph(layout, layout.length - 8), size_t{1});  // b
    EXPECT_EQ(move_to_prev_glyph(layout, layout.length - 9), size_t{1});  // a
    EXPECT_EQ(move_to_prev_glyph(layout, layout.length - 10), size_t{0});
}

TEST(MovementTest, MoveToNextGlyph) {
    auto layout = CreateLayout("abc🙂def");
    EXPECT_EQ(move_to_next_glyph(layout, 0), size_t{1});  // a
    EXPECT_EQ(move_to_next_glyph(layout, 1), size_t{1});  // b
    EXPECT_EQ(move_to_next_glyph(layout, 2), size_t{1});  // c
    EXPECT_EQ(move_to_next_glyph(layout, 3), size_t{4});  // 🙂
    EXPECT_EQ(move_to_next_glyph(layout, 7), size_t{1});  // d
    EXPECT_EQ(move_to_next_glyph(layout, 8), size_t{1});  // e
    EXPECT_EQ(move_to_next_glyph(layout, 9), size_t{1});  // f
    EXPECT_EQ(move_to_next_glyph(layout, 10), size_t{0});
}

TEST(MovementTest, PrevWordStart1) {
    PieceTree tree{"abc🙂def"};
    EXPECT_EQ(prev_word_start(tree, 10), size_t{7});
    EXPECT_EQ(prev_word_start(tree, 9), size_t{7});
    EXPECT_EQ(prev_word_start(tree, 8), size_t{7});
    EXPECT_EQ(prev_word_start(tree, 7), size_t{3});
    EXPECT_EQ(prev_word_start(tree, 3), size_t{0});
    EXPECT_EQ(prev_word_start(tree, 2), size_t{0});
    EXPECT_EQ(prev_word_start(tree, 1), size_t{0});
    EXPECT_EQ(prev_word_start(tree, 0), size_t{0});
}

TEST(MovementTest, PrevWordStart2) {
    PieceTree tree{"🙂🙂🙂"};
    EXPECT_EQ(prev_word_start(tree, 0), size_t{0});
    EXPECT_EQ(prev_word_start(tree, 4), size_t{0});
    EXPECT_EQ(prev_word_start(tree, 8), size_t{0});
    EXPECT_EQ(prev_word_start(tree, tree.length()), size_t{0});
}

TEST(MovementTest, NextWordEnd1) {
    PieceTree tree{"abc🙂def"};
    EXPECT_EQ(next_word_end(tree, 0), size_t{3});
    EXPECT_EQ(next_word_end(tree, 1), size_t{3});
    EXPECT_EQ(next_word_end(tree, 2), size_t{3});
    EXPECT_EQ(next_word_end(tree, 3), size_t{7});
    EXPECT_EQ(next_word_end(tree, 7), size_t{10});
    EXPECT_EQ(next_word_end(tree, 8), size_t{10});
    EXPECT_EQ(next_word_end(tree, 9), size_t{10});
    EXPECT_EQ(next_word_end(tree, 10), size_t{10});
}

TEST(MovementTest, NextWordEnd2) {
    PieceTree tree{"🙂🙂🙂"};
    EXPECT_EQ(next_word_end(tree, 0), tree.length());
    EXPECT_EQ(next_word_end(tree, 4), tree.length());
    EXPECT_EQ(next_word_end(tree, 8), tree.length());
    EXPECT_EQ(next_word_end(tree, tree.length()), tree.length());
}

}  // namespace editor
