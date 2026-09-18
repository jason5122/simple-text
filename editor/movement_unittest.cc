#include "editor/movement.h"
#include "fx/fx.h"
#include <gtest/gtest.h>
#include <memory>

namespace editor {

namespace {
LineLayout CreateLayout(std::string_view str) {
    static const std::unique_ptr<fx_font> font = fx_create_font("system", 32.0f, 0);
    return layout_line(0, *font, 1.0f, str);
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
    EXPECT_EQ(column_at_x(layout, 0), 0UZ);
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
        EXPECT_EQ(move_to_prev_glyph(layout1, 0), 0UZ);
        EXPECT_EQ(move_to_next_glyph(layout1, layout1.length), 0UZ);
    }
    auto layout2 = CreateLayout("");
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(move_to_prev_glyph(layout2, 0), 0UZ);
        EXPECT_EQ(move_to_next_glyph(layout2, 0), 0UZ);
    }
}

// Ensure moving while at the beginning/end does nothing.
TEST(MovementTest, PreventMovingByWordAtBeginningOrEnd) {
    PieceTree tree1{"abc🙂def"};
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(prev_word_start(tree1, 0), 0UZ);
        EXPECT_EQ(next_word_end(tree1, tree1.length()), tree1.length());
    }
    PieceTree tree2{""};
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(prev_word_start(tree2, 0), 0UZ);
        EXPECT_EQ(next_word_end(tree2, 0), 0UZ);
    }
}

TEST(MovementTest, MoveToPrevGlyph) {
    auto layout = CreateLayout("abc🙂def");
    EXPECT_EQ(move_to_prev_glyph(layout, layout.length), 1UZ);      // f
    EXPECT_EQ(move_to_prev_glyph(layout, layout.length - 1), 1UZ);  // e
    EXPECT_EQ(move_to_prev_glyph(layout, layout.length - 2), 1UZ);  // d
    EXPECT_EQ(move_to_prev_glyph(layout, layout.length - 3), 4UZ);  // 🙂
    EXPECT_EQ(move_to_prev_glyph(layout, layout.length - 7), 1UZ);  // c
    EXPECT_EQ(move_to_prev_glyph(layout, layout.length - 8), 1UZ);  // b
    EXPECT_EQ(move_to_prev_glyph(layout, layout.length - 9), 1UZ);  // a
    EXPECT_EQ(move_to_prev_glyph(layout, layout.length - 10), 0UZ);
}

TEST(MovementTest, MoveToNextGlyph) {
    auto layout = CreateLayout("abc🙂def");
    EXPECT_EQ(move_to_next_glyph(layout, 0), 1UZ);  // a
    EXPECT_EQ(move_to_next_glyph(layout, 1), 1UZ);  // b
    EXPECT_EQ(move_to_next_glyph(layout, 2), 1UZ);  // c
    EXPECT_EQ(move_to_next_glyph(layout, 3), 4UZ);  // 🙂
    EXPECT_EQ(move_to_next_glyph(layout, 7), 1UZ);  // d
    EXPECT_EQ(move_to_next_glyph(layout, 8), 1UZ);  // e
    EXPECT_EQ(move_to_next_glyph(layout, 9), 1UZ);  // f
    EXPECT_EQ(move_to_next_glyph(layout, 10), 0UZ);
}

TEST(MovementTest, PrevWordStart1) {
    PieceTree tree{"abc🙂def"};
    EXPECT_EQ(prev_word_start(tree, 10), 7UZ);
    EXPECT_EQ(prev_word_start(tree, 9), 7UZ);
    EXPECT_EQ(prev_word_start(tree, 8), 7UZ);
    EXPECT_EQ(prev_word_start(tree, 7), 3UZ);
    EXPECT_EQ(prev_word_start(tree, 3), 0UZ);
    EXPECT_EQ(prev_word_start(tree, 2), 0UZ);
    EXPECT_EQ(prev_word_start(tree, 1), 0UZ);
    EXPECT_EQ(prev_word_start(tree, 0), 0UZ);
}

TEST(MovementTest, PrevWordStart2) {
    PieceTree tree{"🙂🙂🙂"};
    EXPECT_EQ(prev_word_start(tree, 0), 0UZ);
    EXPECT_EQ(prev_word_start(tree, 4), 0UZ);
    EXPECT_EQ(prev_word_start(tree, 8), 0UZ);
    EXPECT_EQ(prev_word_start(tree, tree.length()), 0UZ);
}

TEST(MovementTest, NextWordEnd1) {
    PieceTree tree{"abc🙂def"};
    EXPECT_EQ(next_word_end(tree, 0), 3UZ);
    EXPECT_EQ(next_word_end(tree, 1), 3UZ);
    EXPECT_EQ(next_word_end(tree, 2), 3UZ);
    EXPECT_EQ(next_word_end(tree, 3), 7UZ);
    EXPECT_EQ(next_word_end(tree, 7), 10UZ);
    EXPECT_EQ(next_word_end(tree, 8), 10UZ);
    EXPECT_EQ(next_word_end(tree, 9), 10UZ);
    EXPECT_EQ(next_word_end(tree, 10), 10UZ);
}

TEST(MovementTest, NextWordEnd2) {
    PieceTree tree{"🙂🙂🙂"};
    EXPECT_EQ(next_word_end(tree, 0), tree.length());
    EXPECT_EQ(next_word_end(tree, 4), tree.length());
    EXPECT_EQ(next_word_end(tree, 8), tree.length());
    EXPECT_EQ(next_word_end(tree, tree.length()), tree.length());
}

}  // namespace editor
