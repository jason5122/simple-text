#include "base/buffer/piece_tree.h"
#include "base/numeric/literals.h"
#include "third_party/uni_algo/include/uni_algo/prop.h"
#include <gtest/gtest.h>

// TODO: Debug use; remove this.
#include "util/std_print.h"

namespace base {

TEST(TreeWalkerTest, TreeWalkerNextUTF8Test1) {
    PieceTree tree{"abcdefghijklmnopqrstuvwxyz"};
    TreeWalker walker{&tree};

    while (!walker.exhausted()) {
        int32_t codepoint = walker.next_codepoint();
        EXPECT_TRUE(una::codepoint::is_alphabetic(codepoint));
    }
    EXPECT_EQ(walker.next_codepoint(), 0);
}

TEST(TreeWalkerTest, TreeWalkerNextUTF8Test2) {
    PieceTree tree1{"﷽"};
    TreeWalker walker1{&tree1};
    EXPECT_EQ(walker1.next_codepoint(), static_cast<int32_t>(U'\U0000FDFD'));
    EXPECT_TRUE(walker1.exhausted());

    PieceTree tree2{"☃️"};
    TreeWalker walker2{&tree2};
    EXPECT_EQ(walker2.next_codepoint(), static_cast<int32_t>(U'\U00002603'));
    EXPECT_FALSE(walker2.exhausted());
    EXPECT_EQ(walker2.next_codepoint(), static_cast<int32_t>(U'\U0000FE0F'));
    EXPECT_TRUE(walker2.exhausted());
}

TEST(TreeWalkerTest, ReverseTreeWalkerNextUTF8Test1) {
    PieceTree tree{"abcdefghijklmnopqrstuvwxyz"};
    ReverseTreeWalker walker{&tree};

    while (!walker.exhausted()) {
        int32_t codepoint = walker.next_codepoint();
        EXPECT_TRUE(una::codepoint::is_alphabetic(codepoint));
    }
    EXPECT_EQ(walker.next_codepoint(), 0);
}

TEST(TreeWalkerTest, ReverseTreeWalkerNextUTF8Test2) {
    PieceTree tree1{"﷽"};
    ReverseTreeWalker walker1{&tree1, tree1.length()};
    EXPECT_EQ(walker1.next_codepoint(), static_cast<int32_t>(U'\U0000FDFD'));
    EXPECT_TRUE(walker1.exhausted());

    PieceTree tree2{"☃️"};
    ReverseTreeWalker walker2{&tree2, tree2.length()};
    EXPECT_EQ(walker2.next_codepoint(), static_cast<int32_t>(U'\U0000FE0F'));
    EXPECT_FALSE(walker2.exhausted());
    EXPECT_EQ(walker2.next_codepoint(), static_cast<int32_t>(U'\U00002603'));
    EXPECT_TRUE(walker2.exhausted());
}

TEST(TreeWalkerTest, ReverseTreeWalkerNextUTF8Test3) {
    PieceTree tree{"a bc\u205Fxyz"};
    ReverseTreeWalker walker{&tree, tree.length()};
    EXPECT_EQ(walker.next_codepoint(), static_cast<int32_t>(U'z'));
    EXPECT_EQ(walker.next_codepoint(), static_cast<int32_t>(U'y'));
    EXPECT_EQ(walker.next_codepoint(), static_cast<int32_t>(U'x'));
    EXPECT_EQ(walker.next_codepoint(), static_cast<int32_t>(U'\u205F'));
    EXPECT_EQ(walker.next_codepoint(), static_cast<int32_t>(U'c'));
    EXPECT_EQ(walker.next_codepoint(), static_cast<int32_t>(U'b'));
    EXPECT_EQ(walker.next_codepoint(), static_cast<int32_t>(U' '));
    EXPECT_EQ(walker.next_codepoint(), static_cast<int32_t>(U'a'));
    EXPECT_EQ(walker.next_codepoint(), 0);
    EXPECT_TRUE(walker.exhausted());
}

TEST(TreeWalkerTest, ReverseTreeWalkerNextUTF8Test4) {
    PieceTree tree{"a bc\u205Fxyz"};
    ReverseTreeWalker walker{&tree, 3};
    EXPECT_EQ(walker.next_codepoint(), static_cast<int32_t>(U'c'));
    EXPECT_EQ(walker.next_codepoint(), static_cast<int32_t>(U'b'));
    EXPECT_EQ(walker.next_codepoint(), static_cast<int32_t>(U' '));
    EXPECT_EQ(walker.next_codepoint(), static_cast<int32_t>(U'a'));
    EXPECT_EQ(walker.next_codepoint(), 0);
    EXPECT_TRUE(walker.exhausted());
}

TEST(TreeWalkerTest, TreeWalkerOffsetTest1) {
    std::string str = "012345";
    PieceTree tree{str};

    for (size_t start = 0; start < tree.length(); ++start) {
        size_t i = start;
        TreeWalker walker{&tree, start};
        while (!walker.exhausted()) {
            EXPECT_EQ(walker.offset(), i);
            EXPECT_EQ(walker.next(), str[i]);
            ++i;
        }
        EXPECT_EQ(walker.offset(), tree.length());
    }
}

TEST(TreeWalkerTest, TreeWalkerOffsetTest2) {
    PieceTree tree{"abcd"};
    TreeWalker walker{&tree};

    EXPECT_EQ(walker.offset(), 0_Z);
    EXPECT_FALSE(walker.exhausted());

    walker = {&tree, 4};
    EXPECT_EQ(walker.offset(), tree.length());
    EXPECT_TRUE(walker.exhausted());

    walker = {&tree, 100};
    EXPECT_EQ(walker.offset(), tree.length());
    EXPECT_TRUE(walker.exhausted());
}

TEST(TreeWalkerTest, ReverseTreeWalkerOffsetTest1) {
    std::string str = "012345";
    PieceTree tree{str};

    for (size_t start = 0; start < tree.length(); ++start) {
        size_t i = start;
        ReverseTreeWalker walker{&tree, start};
        while (!walker.exhausted()) {
            EXPECT_EQ(walker.offset(), i);
            EXPECT_EQ(walker.next(), str[i]);
            --i;
        }
        // TODO: Consider changing wrapping behavior of `ReverseTreeWalker::next()`.
        EXPECT_EQ(walker.offset(), std::numeric_limits<size_t>::max());
    }
}

TEST(TreeWalkerTest, ReverseTreeWalkerOffsetTest2) {
    PieceTree tree{"abcd"};
    ReverseTreeWalker walker{&tree};

    EXPECT_EQ(walker.offset(), 0_Z);
    EXPECT_FALSE(walker.exhausted());
    EXPECT_EQ(walker.next(), 'a');
    EXPECT_TRUE(walker.exhausted());

    walker = {&tree, 3};
    EXPECT_EQ(walker.offset(), 3_Z);
    EXPECT_FALSE(walker.exhausted());
    EXPECT_EQ(walker.next(), 'd');
    EXPECT_EQ(walker.next(), 'c');
    EXPECT_EQ(walker.next(), 'b');
    EXPECT_EQ(walker.next(), 'a');
    EXPECT_TRUE(walker.exhausted());

    walker = {&tree, 4};
    EXPECT_EQ(walker.offset(), tree.length());
    EXPECT_EQ(walker.next(), 'd');

    walker = {&tree, 100};
    EXPECT_EQ(walker.offset(), tree.length());
    EXPECT_EQ(walker.next(), 'd');
}

// Check that the forward and reverse walkers have the same offsets and codepoints.
TEST(TreeWalkerTest, UTF8OffsetsAndCodepointsMatch) {
    PieceTree tree{"abc🙂def"};

    auto walker = TreeWalker{&tree};
    auto reverse_walker = ReverseTreeWalker{&tree, tree.length()};

    std::stack<size_t> offset_stk;
    std::stack<int32_t> cp_stk;

    while (!walker.exhausted()) {
        size_t offset = walker.offset();
        int32_t cp = walker.next_codepoint();

        offset_stk.push(offset);
        cp_stk.push(cp);
    }

    while (!reverse_walker.exhausted()) {
        int32_t cp = reverse_walker.next_codepoint();
        size_t offset = reverse_walker.offset();

        EXPECT_EQ(offset, offset_stk.top());
        EXPECT_EQ(cp, cp_stk.top());
        offset_stk.pop();
        cp_stk.pop();
    }

    EXPECT_TRUE(offset_stk.empty());
    EXPECT_TRUE(cp_stk.empty());
}

}  // namespace base
