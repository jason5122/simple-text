#include "editor/buffer/red_black_tree.h"
#include <gtest/gtest.h>

namespace editor {

using Tree = RedBlackTree;
using Color = RedBlackTree::Color;

namespace {

bool tree_shape_equal(const RedBlackTree& a, const RedBlackTree& b) {
    if (!a && !b) return true;
    if (!a || !b) return false;
    if (a.color() != b.color()) return false;
    return tree_shape_equal(a.left(), b.left()) && tree_shape_equal(a.right(), b.right());
}
testing::AssertionResult assert_tree_shape_equal(const char* a_expr,
                                                 const char* b_expr,
                                                 const RedBlackTree& a,
                                                 const RedBlackTree& b) {
    if (tree_shape_equal(a, b)) return testing::AssertionSuccess();
    return testing::AssertionFailure() << "Trees not equal: " << a_expr << " vs " << b_expr;
}
#define EXPECT_TREE_EQ(a, b) EXPECT_PRED_FORMAT2(assert_tree_shape_equal, a, b)

Tree B(const Tree& L, const Tree& R) { return {Color::Black, L, {}, R}; }
Tree R(const Tree& L, const Tree& R) { return {Color::Red, L, {}, R}; }
Tree BL() { return {Color::Black, {}, {}, {}}; }
Tree RL() { return {Color::Red, {}, {}, {}}; }
Tree NIL() { return {}; }

}  // namespace

TEST(RedBlackTreeTest, Constructor) {
    Tree t1;
    EXPECT_FALSE(t1);
    EXPECT_TRUE(t1.is_black());
    EXPECT_EQ(t1.length(), 0);
    EXPECT_EQ(t1.line_feed_count(), 0);

    Tree t2 = {Color::Red, {}, {.length = 10, .lf_count = 5}, {}};
    EXPECT_TRUE(t2);
    EXPECT_TRUE(t2.is_red());
    EXPECT_FALSE(t2.left());
    EXPECT_FALSE(t2.right());
    EXPECT_EQ(t2.length(), 10);
    EXPECT_EQ(t2.line_feed_count(), 5);
}

TEST(RedBlackTreeTest, SatisfiesRedBlackInvariants) {
    auto valid = std::to_array<Tree>({
        NIL(),
        BL(),
        RL(),
        B(BL(), BL()),
        B(RL(), RL()),
        B(B(RL(), RL()), BL()),
        B(B(BL(), BL()), B(BL(), BL())),
    });
    for (auto t : valid) EXPECT_TRUE(t.satisfies_red_black_invariants());

    // Red node cannot have a red child.
    auto red_violation = std::to_array<Tree>({
        R(NIL(), RL()),
        B(R(RL(), NIL()), BL()),
    });
    for (auto t : red_violation) EXPECT_FALSE(t.satisfies_red_black_invariants());

    // Black heights must be the same across all paths.
    auto black_violation = std::to_array<Tree>({
        B(BL(), NIL()),
        B(B(NIL(), BL()), NIL()),
        B(RL(), BL()),
        B(B(BL(), BL()), B(BL(), RL())),
    });
    for (auto t : black_violation) EXPECT_FALSE(t.satisfies_red_black_invariants());
}

TEST(RedBlackTreeTest, Balance) {
    // See okasaki_balance.png (Okasaki, 1999, Fig. 1) for the balance cases.
    auto okasaki_cases = std::to_array<Tree>({
        B(R(RL(), NIL()), NIL()),  // LL
        B(R(NIL(), RL()), NIL()),  // LR
        B(NIL(), R(RL(), NIL())),  // RL
        B(NIL(), R(NIL(), RL())),  // RR
    });
    auto balanced = R(BL(), BL());
    for (const auto& t : okasaki_cases) {
        EXPECT_TREE_EQ(t.balance(), balanced);
        EXPECT_FALSE(t.satisfies_red_black_invariants());
    }

    auto no_op = std::to_array<Tree>({
        NIL(),
        BL(),
        RL(),
        B(BL(), BL()),
        R(BL(), BL()),
        B(B(BL(), BL()), B(BL(), BL())),
    });
    for (const auto& t : no_op) {
        EXPECT_TREE_EQ(t, t.balance());
        EXPECT_TRUE(t.satisfies_red_black_invariants());
    }
}

// TEST(RedBlackTreeTest, Insert) {
//     Tree t;
//     t = t.insert(0, P(5, 2));
//     EXPECT_EQ(t.length(), 5);
//     EXPECT_EQ(t.line_feed_count(), 2);

//     t = t.insert(0, P(10, 0));
//     EXPECT_EQ(t.length(), 15);
//     EXPECT_EQ(t.line_feed_count(), 2);

//     t = t.insert(10, P(1, 1));
//     EXPECT_EQ(t.length(), 16);
//     EXPECT_EQ(t.line_feed_count(), 3);
// }

// TEST(RedBlackTreeTest, RandomInserts) {
//     Tree t;
//     size_t total_len = 0;
//     size_t total_lf = 0;

//     for (size_t i = 0; i < 1000; ++i) {
//         size_t at = base::rand_generator(total_len + 1);
//         size_t len = base::rand_int(1, 100);
//         size_t lf = base::rand_generator(len);
//         t = t.insert(at, P(len, lf));

//         total_len += len;
//         total_lf += lf;
//     }

//     EXPECT_EQ(t.length(), total_len);
//     EXPECT_EQ(t.line_feed_count(), total_lf);
// }

}  // namespace editor
