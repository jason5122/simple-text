#include "editor/buffer/red_black_tree.h"
#include <gtest/gtest.h>

namespace editor {

TEST(RedBlackTreeTest, Constructor) {
    RedBlackTree tree;
    EXPECT_FALSE(tree);
    EXPECT_EQ(tree.length(), 0);
    EXPECT_EQ(tree.line_feed_count(), 0);
}

}  // namespace editor
