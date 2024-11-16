#include "base/buffer/piece_tree.h"
#include "third_party/uni_algo/include/uni_algo/prop.h"
#include <gtest/gtest.h>

// TODO: Debug use; remove this.
#include "util/std_print.h"

namespace base {

TEST(TreeWalkerTest, TreeWalkerNextUTF8Test1) {
    PieceTree tree{"abcdefghijklmnopqrstuvwxyz"};
    TreeWalker walker{&tree};

    while (!walker.exhausted()) {
        int32_t codepoint = walker.nextCodePoint();
        EXPECT_TRUE(una::codepoint::is_alphabetic(codepoint));
    }
    EXPECT_EQ(walker.nextCodePoint(), 0);
}

TEST(TreeWalkerTest, TreeWalkerNextUTF8Test2) {
    PieceTree tree1{"﷽"};
    TreeWalker walker1{&tree1};
    EXPECT_EQ(walker1.nextCodePoint(), static_cast<int32_t>(U'\U0000FDFD'));
    EXPECT_TRUE(walker1.exhausted());

    PieceTree tree2{"☃️"};
    TreeWalker walker2{&tree2};
    EXPECT_EQ(walker2.nextCodePoint(), static_cast<int32_t>(U'\U00002603'));
    EXPECT_FALSE(walker2.exhausted());
    EXPECT_EQ(walker2.nextCodePoint(), static_cast<int32_t>(U'\U0000FE0F'));
    EXPECT_TRUE(walker2.exhausted());
}

}  // namespace base
