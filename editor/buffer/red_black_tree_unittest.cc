#include "base/rand_util.h"
#include "editor/buffer/red_black_tree.h"
#include <gtest/gtest.h>

namespace editor {

using Tree = RedBlackTree;
using Color = RedBlackTree::Color;

namespace {

inline Piece P(size_t len, size_t lf) { return {.length = len, .lf_count = lf}; }

// Black leaf.
inline Tree BL(size_t len = 1, size_t lf = 0) { return {Color::Black, {}, P(len, lf), {}}; }
// Red leaf.
inline Tree RL(size_t len = 1, size_t lf = 0) { return {Color::Red, {}, P(len, lf), {}}; }
// Black node.
inline Tree B(const Tree& L, const Tree& R, size_t len = 1, size_t lf = 0) {
    return {Color::Black, L, P(len, lf), R};
}
// Red node.
inline Tree R(const Tree& L, const Tree& R, size_t len = 1, size_t lf = 0) {
    return {Color::Red, L, P(len, lf), R};
}

}  // namespace

TEST(RedBlackTreeTest, Constructor) {
    Tree t1;
    EXPECT_FALSE(t1);
    EXPECT_EQ(t1.length(), 0);
    EXPECT_EQ(t1.line_feed_count(), 0);

    Tree t2 = BL(10, 5);
    EXPECT_TRUE(t2);
    EXPECT_EQ(t2.color(), Color::Black);
    EXPECT_EQ(t2.left(), Tree{});
    EXPECT_EQ(t2.right(), Tree{});
    EXPECT_EQ(t2.length(), 10);
    EXPECT_EQ(t2.line_feed_count(), 5);
}

TEST(RedBlackTreeTest, Insert) {
    Tree t;
    t = t.insert(0, P(5, 2));
    EXPECT_EQ(t.length(), 5);
    EXPECT_EQ(t.line_feed_count(), 2);

    t = t.insert(0, P(10, 0));
    EXPECT_EQ(t.length(), 15);
    EXPECT_EQ(t.line_feed_count(), 2);

    t = t.insert(10, P(1, 1));
    EXPECT_EQ(t.length(), 16);
    EXPECT_EQ(t.line_feed_count(), 3);
}

TEST(RedBlackTreeTest, RandomInserts) {
    Tree t;
    size_t total_len = 0;
    size_t total_lf = 0;

    for (size_t i = 0; i < 1000; ++i) {
        size_t at = base::rand_generator(total_len + 1);
        size_t len = base::rand_int(1, 100);
        size_t lf = base::rand_generator(len);
        t = t.insert(at, P(len, lf));

        total_len += len;
        total_lf += lf;
    }

    EXPECT_EQ(t.length(), total_len);
    EXPECT_EQ(t.line_feed_count(), total_lf);
}

TEST(RedBlackTreeTest, CheckInvariants) {
    auto kValid = std::to_array<Tree>({
        {},    // NIL.
        BL(),  // Single black.
        RL(),  // Single red.
        B(BL(), BL()),
        B(RL(), RL()),
        B(B(RL(), RL()), BL()),
        B(B(BL(), BL()), B(BL(), BL())),
    });
    for (auto t : kValid) EXPECT_TRUE(t.satisfies_red_black_invariants());

    // Red node cannot have a red child.
    auto kRedViolation = std::to_array<Tree>({
        R({}, RL()),
        B(R(RL(), {}), BL()),
    });
    for (auto t : kRedViolation) EXPECT_FALSE(t.satisfies_red_black_invariants());

    // Black heights must be the same across all paths.
    auto kBlackViolation = std::to_array<Tree>({
        B(BL(), {}),
        B(B({}, BL()), {}),
        B(RL(), BL()),
        B(B(BL(), BL()), B(BL(), RL())),
    });
    for (auto t : kBlackViolation) EXPECT_FALSE(t.satisfies_red_black_invariants());
}

}  // namespace editor
