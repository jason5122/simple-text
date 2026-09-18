#include "editor/buffer/piece_tree.h"
#include "uni_algo/prop.h"
#include <algorithm>
#include <gtest/gtest.h>
#include <stack>
#include <string_view>
#include <utility>
#include <vector>

namespace editor {

TEST(TreeWalkerTest, TreeWalkerNextUTF8Test1) {
    PieceTree tree{"abcdefghijklmnopqrstuvwxyz"};
    TreeWalker walker{tree};

    while (!walker.exhausted()) {
        int32_t codepoint = walker.next_codepoint();
        EXPECT_TRUE(una::codepoint::is_alphabetic(codepoint));
    }
    EXPECT_EQ(walker.next_codepoint(), 0);
}

TEST(TreeWalkerTest, TreeWalkerNextUTF8Test2) {
    PieceTree tree1{"﷽"};
    TreeWalker walker1{tree1};
    EXPECT_EQ(walker1.next_codepoint(), static_cast<int32_t>(U'\U0000FDFD'));
    EXPECT_TRUE(walker1.exhausted());

    PieceTree tree2{"☃️"};
    TreeWalker walker2{tree2};
    EXPECT_EQ(walker2.next_codepoint(), static_cast<int32_t>(U'\U00002603'));
    EXPECT_FALSE(walker2.exhausted());
    EXPECT_EQ(walker2.next_codepoint(), static_cast<int32_t>(U'\U0000FE0F'));
    EXPECT_TRUE(walker2.exhausted());
}

TEST(TreeWalkerTest, ReverseTreeWalkerNextUTF8Test1) {
    PieceTree tree{"abcdefghijklmnopqrstuvwxyz"};
    ReverseTreeWalker walker{tree};

    while (!walker.exhausted()) {
        int32_t codepoint = walker.next_codepoint();
        EXPECT_TRUE(una::codepoint::is_alphabetic(codepoint));
    }
    EXPECT_EQ(walker.next_codepoint(), 0);
}

TEST(TreeWalkerTest, ReverseTreeWalkerNextUTF8Test2) {
    PieceTree tree1{"﷽"};
    ReverseTreeWalker walker1{tree1, tree1.length()};
    EXPECT_EQ(walker1.next_codepoint(), static_cast<int32_t>(U'\U0000FDFD'));
    EXPECT_TRUE(walker1.exhausted());

    PieceTree tree2{"☃️"};
    ReverseTreeWalker walker2{tree2, tree2.length()};
    EXPECT_EQ(walker2.next_codepoint(), static_cast<int32_t>(U'\U0000FE0F'));
    EXPECT_FALSE(walker2.exhausted());
    EXPECT_EQ(walker2.next_codepoint(), static_cast<int32_t>(U'\U00002603'));
    EXPECT_TRUE(walker2.exhausted());
}

TEST(TreeWalkerTest, ReverseTreeWalkerNextUTF8Test3) {
    PieceTree tree{"a bc\u205Fxyz"};
    ReverseTreeWalker walker{tree, tree.length()};
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
    ReverseTreeWalker walker{tree, 4};
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
        TreeWalker walker{tree, start};
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
    TreeWalker w1{tree};

    EXPECT_EQ(w1.offset(), 0UZ);
    EXPECT_FALSE(w1.exhausted());

    TreeWalker w2 = {tree, 4};
    EXPECT_EQ(w2.offset(), tree.length());
    EXPECT_TRUE(w2.exhausted());

    TreeWalker w3 = {tree, 100};
    EXPECT_EQ(w3.offset(), tree.length());
    EXPECT_TRUE(w3.exhausted());
}

TEST(TreeWalkerTest, Exhausted) {
    PieceTree tree{"abcd"};

    TreeWalker w1 = {tree, tree.length()};
    EXPECT_TRUE(w1.exhausted());
    TreeWalker w2 = {tree, 1};
    EXPECT_FALSE(w2.exhausted());

    ReverseTreeWalker reverse_walker{tree};
    ReverseTreeWalker rw1 = {tree, 0};
    EXPECT_TRUE(rw1.exhausted());
    ReverseTreeWalker rw2 = {tree, 1};
    EXPECT_FALSE(rw2.exhausted());
}

TEST(TreeWalkerTest, ReverseTreeWalkerOffsetTest1) {
    std::string str = "012345";
    PieceTree tree{str};

    for (size_t start = 0; start <= tree.length(); ++start) {
        size_t i = start;
        ReverseTreeWalker reverse_walker{tree, start};
        while (!reverse_walker.exhausted()) {
            char ch = reverse_walker.next();
            size_t offset = reverse_walker.offset();
            --i;

            EXPECT_EQ(ch, str[i]);
            EXPECT_EQ(offset, i);
        }
        EXPECT_EQ(reverse_walker.offset(), 0UZ);
    }
}

TEST(TreeWalkerTest, ReverseTreeWalkerOffsetTest2) {
    PieceTree tree{"abcd"};

    ReverseTreeWalker rw1{tree};
    EXPECT_EQ(rw1.offset(), 0UZ);
    EXPECT_TRUE(rw1.exhausted());

    ReverseTreeWalker rw2 = {tree, 3};  // abc|d
    EXPECT_EQ(rw2.offset(), 3UZ);
    EXPECT_FALSE(rw2.exhausted());
    EXPECT_EQ(rw2.next(), 'c');
    EXPECT_EQ(rw2.next(), 'b');
    EXPECT_EQ(rw2.next(), 'a');
    EXPECT_TRUE(rw2.exhausted());

    ReverseTreeWalker rw3 = {tree, 4};  // abcd|
    EXPECT_EQ(rw3.offset(), tree.length());
    EXPECT_EQ(rw3.next(), 'd');

    ReverseTreeWalker rw4 = {tree, 100};  // abcd|
    EXPECT_EQ(rw4.offset(), tree.length());
    EXPECT_EQ(rw4.next(), 'd');
}

// Check that the forward and reverse walkers have the same offsets and codepoints.
TEST(TreeWalkerTest, UTF8OffsetsAndCodepointsMatch) {
    PieceTree tree{"abc🙂def"};

    auto walker = TreeWalker{tree};
    auto reverse_walker = ReverseTreeWalker{tree, tree.length()};

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

// Offsets/codepoints should be the same no matter which offset the walker starts on.
TEST(TreeWalkerTest, WalkerConsistency) {
    PieceTree tree{"abc🙂def"};

    auto get_codepoints = [](const PieceTree& tree, size_t offset) {
        std::vector<int32_t> codepoints;
        auto walker = TreeWalker{tree, offset};
        while (!walker.exhausted()) {
            int32_t cp = walker.next_codepoint();
            codepoints.emplace_back(cp);
        }
        return codepoints;
    };

    std::vector<int32_t> codepoints, expected;

    codepoints = get_codepoints(tree, 0);
    expected = {0x61, 0x62, 0x63, 0x1F642, 0x64, 0x65, 0x66};
    EXPECT_EQ(codepoints, expected);

    codepoints = get_codepoints(tree, 1);
    expected = {0x62, 0x63, 0x1F642, 0x64, 0x65, 0x66};
    EXPECT_EQ(codepoints, expected);

    codepoints = get_codepoints(tree, 2);
    expected = {0x63, 0x1F642, 0x64, 0x65, 0x66};
    EXPECT_EQ(codepoints, expected);

    codepoints = get_codepoints(tree, 3);
    expected = {0x1F642, 0x64, 0x65, 0x66};
    EXPECT_EQ(codepoints, expected);

    codepoints = get_codepoints(tree, 7);
    expected = {0x64, 0x65, 0x66};
    EXPECT_EQ(codepoints, expected);

    codepoints = get_codepoints(tree, 8);
    expected = {0x65, 0x66};
    EXPECT_EQ(codepoints, expected);

    codepoints = get_codepoints(tree, 9);
    expected = {0x66};
    EXPECT_EQ(codepoints, expected);

    codepoints = get_codepoints(tree, tree.length());
    expected = {};
    EXPECT_EQ(codepoints, expected);
}

// Offsets/codepoints should be the same no matter which offset the walker starts on.
TEST(TreeWalkerTest, ReverseWalkerConsistency) {
    PieceTree tree{"abc🙂def"};

    auto get_codepoints = [](const PieceTree& tree, size_t offset) {
        std::vector<int32_t> codepoints;
        auto reverse_walker = ReverseTreeWalker{tree, offset};
        while (!reverse_walker.exhausted()) {
            int32_t cp = reverse_walker.next_codepoint();
            codepoints.emplace_back(cp);
        }
        return codepoints;
    };

    std::vector<int32_t> codepoints, expected;

    codepoints = get_codepoints(tree, tree.length());
    expected = {0x66, 0x65, 0x64, 0x1F642, 0x63, 0x62, 0x61};
    EXPECT_EQ(codepoints, expected);

    codepoints = get_codepoints(tree, 9);
    expected = {0x65, 0x64, 0x1F642, 0x63, 0x62, 0x61};
    EXPECT_EQ(codepoints, expected);

    codepoints = get_codepoints(tree, 8);
    expected = {0x64, 0x1F642, 0x63, 0x62, 0x61};
    EXPECT_EQ(codepoints, expected);

    codepoints = get_codepoints(tree, 7);
    expected = {0x1F642, 0x63, 0x62, 0x61};
    EXPECT_EQ(codepoints, expected);

    codepoints = get_codepoints(tree, 3);
    expected = {0x63, 0x62, 0x61};
    EXPECT_EQ(codepoints, expected);

    codepoints = get_codepoints(tree, 2);
    expected = {0x62, 0x61};
    EXPECT_EQ(codepoints, expected);

    codepoints = get_codepoints(tree, 1);
    expected = {0x61};
    EXPECT_EQ(codepoints, expected);

    codepoints = get_codepoints(tree, 0);
    expected = {};
    EXPECT_EQ(codepoints, expected);
}

namespace {

size_t piece_count(const RedBlackTree& node) {
    if (!node) return 0;
    return 1 + piece_count(node.left()) + piece_count(node.right());
}

// Makes `offset` a piece boundary by splitting the piece that contains it.
void split_at(PieceTree& tree, size_t offset) {
    tree.insert(offset, "x");
    tree.erase(offset, 1);
}

// (byte offset, code point) per code point, in text order.
using Codepoints = std::vector<std::pair<size_t, char32_t>>;

Codepoints forward_codepoints(const PieceTree& tree) {
    Codepoints codepoints;
    TreeWalker walker{tree};
    while (!walker.exhausted()) {
        size_t offset = walker.offset();
        char32_t cp = walker.next_codepoint();
        codepoints.emplace_back(offset, cp);
    }
    return codepoints;
}

Codepoints reverse_codepoints(const PieceTree& tree) {
    Codepoints codepoints;
    ReverseTreeWalker walker{tree, tree.length()};
    while (!walker.exhausted()) {
        char32_t cp = walker.next_codepoint();
        codepoints.emplace_back(walker.offset(), cp);
    }
    std::ranges::reverse(codepoints);
    return codepoints;
}

}  // namespace

// Both walkers decode the same code points at the same offsets no matter where the piece
// boundaries fall, including inside multi-byte sequences.
TEST(TreeWalkerTest, CodepointsStraddlingPieceBoundaries) {
    // "a" | U+1F642 (4 bytes) | "b" | U+2603 (3 bytes) | U+00E9 (2 bytes) | "c"
    const std::string_view text = "a🙂b☃\u00E9c";
    const Codepoints expected = {{0, U'a'},      {1, U'\U0001F642'}, {5, U'b'},
                                 {6, U'\u2603'}, {9, U'\u00E9'},     {11, U'c'}};

    // A piece boundary after every `stride` bytes. Stride 1 splits every multi-byte sequence at
    // every byte; the largest stride leaves a single piece.
    for (size_t stride = 1; stride <= text.size(); ++stride) {
        PieceTree tree{text};
        for (size_t offset = stride; offset < text.size(); offset += stride) {
            split_at(tree, offset);
        }
        ASSERT_EQ(tree.str(), text);
        ASSERT_EQ(piece_count(tree.root()), (text.size() + stride - 1) / stride);

        EXPECT_EQ(forward_codepoints(tree), expected) << "stride " << stride;
        EXPECT_EQ(reverse_codepoints(tree), expected) << "stride " << stride;
    }
}

// Ill-formed bytes decode to U+FFFD in both directions, and both directions agree on where each
// code point starts, whether or not a piece boundary falls inside the ill-formed sequence.
TEST(TreeWalkerTest, IllFormedBytesDecodeToReplacementCharacter) {
    // "a" | stray trail byte | truncated 3-byte sequence | "b" | lead byte with no trail | "A"
    // clang-format off
    const std::string_view text = "a\x80\xE2\x82" "b\xE2" "A";
    // clang-format on
    const Codepoints expected = {{0, U'a'}, {1, U'\uFFFD'}, {2, U'\uFFFD'},
                                 {4, U'b'}, {5, U'\uFFFD'}, {6, U'A'}};

    for (size_t stride = 1; stride <= text.size(); ++stride) {
        PieceTree tree{text};
        for (size_t offset = stride; offset < text.size(); offset += stride) {
            split_at(tree, offset);
        }
        ASSERT_EQ(tree.str(), text);

        EXPECT_EQ(forward_codepoints(tree), expected) << "stride " << stride;
        EXPECT_EQ(reverse_codepoints(tree), expected) << "stride " << stride;
    }
}

}  // namespace editor
