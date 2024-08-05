#include "base/numeric/wrap_arithmetic.h"
#include "gtest/gtest.h"
#include <cstddef>
#include <limits>

namespace {

constexpr int min_int = std::numeric_limits<int>::min();
constexpr int max_int = std::numeric_limits<int>::max();
constexpr size_t min_size_t = std::numeric_limits<size_t>::min();
constexpr size_t max_size_t = std::numeric_limits<size_t>::max();

}

namespace base {
TEST(WrapArithmeticTest, IncWrap) {
    EXPECT_EQ(inc_wrap(0UL, 3UL), 1UL);
    EXPECT_EQ(inc_wrap(1UL, 3UL), 2UL);
    EXPECT_EQ(inc_wrap(2UL, 3UL), 0UL);
    EXPECT_EQ(inc_wrap(0UL, 1UL), 0UL);
    EXPECT_EQ(inc_wrap(max_size_t - 1, max_size_t), 0UL);
}

TEST(WrapArithmeticTest, DecWrap) {
    EXPECT_EQ(dec_wrap(0UL, 3UL), 2UL);
    EXPECT_EQ(dec_wrap(1UL, 3UL), 0UL);
    EXPECT_EQ(dec_wrap(2UL, 3UL), 1UL);
    EXPECT_EQ(dec_wrap(0UL, 1UL), 0UL);
    EXPECT_EQ(dec_wrap(0, max_size_t), max_size_t - 1);
}

}
