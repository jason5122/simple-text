#include "base/rand_util.h"
#include "editor/buffer/red_black_tree.h"
#include <gtest/gtest.h>

namespace editor {

TEST(RedBlackTreePerfTest, Insertions) {
    RedBlackTree t;
    for (size_t i = 0; i < 100'000; i++) {
        size_t at = base::rand_int(0, t.length());
        Piece p = {.length = 1};
        t = t.insert(at, p);
    }
}

TEST(RedBlackTreePerfTest, Deletions) {
    constexpr size_t N = 100'000;
    RedBlackTree t;
    for (size_t i = 0; i < N; i++) {
        size_t at = base::rand_int(0, t.length());
        Piece p = {.length = 1};
        t = t.insert(at, p);
    }
    ASSERT_EQ(t.length(), N);

    for (size_t i = 0; i < N; i++) {
        ASSERT_GT(t.length(), 0);
        size_t at = base::rand_int(0, t.length() - 1);
        t = t.remove(at);
    }
    EXPECT_FALSE(t);
}

}  // namespace editor
