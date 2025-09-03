#include "base/numeric/saturation_arithmetic.h"
#include <gtest/gtest.h>

namespace {

static_assert(std::is_unsigned<int>::value == false);
static_assert(std::is_unsigned<size_t>::value == true);

constexpr int min_int = std::numeric_limits<int>::min();
constexpr int max_int = std::numeric_limits<int>::max();
constexpr size_t min_size_t = std::numeric_limits<size_t>::min();
constexpr size_t max_size_t = std::numeric_limits<size_t>::max();

}  // namespace

namespace base {

TEST(SaturationArithmeticTest, AddSatNoOverflow) {
    EXPECT_EQ(add_sat(10, 5), 15);
    EXPECT_EQ(add_sat(-10, 5), -5);
    EXPECT_EQ(add_sat(10UL, 5UL), 15UL);
}

TEST(SaturationArithmeticTest, AddSatNoOverflowBoundaries) {
    EXPECT_EQ(add_sat(0, min_int), min_int);
    EXPECT_EQ(add_sat(size_t{0}, min_size_t), min_size_t);
    EXPECT_EQ(add_sat(max_int - 5, 5), max_int);
    EXPECT_EQ(add_sat(min_int + 5, -5), min_int);
    EXPECT_EQ(add_sat(max_size_t - size_t{5}, size_t{5}), max_size_t);
}

TEST(SaturationArithmeticTest, AddSatOverflow) {
    EXPECT_EQ(add_sat(max_int, 1), max_int);
    EXPECT_EQ(add_sat(max_size_t, size_t{1}), max_size_t);
}

TEST(SaturationArithmeticTest, SubSatNoOverflow) {
    EXPECT_EQ(sub_sat(10, 5), 5);
    EXPECT_EQ(sub_sat(-10, 5), -15);
    EXPECT_EQ(sub_sat(10UL, 5UL), 5UL);
}

TEST(SaturationArithmeticTest, SubSatNoOverflowBoundaries) {
    EXPECT_EQ(sub_sat(max_int, 0), max_int);
    EXPECT_EQ(sub_sat(max_size_t, size_t{0}), max_size_t);
    EXPECT_EQ(sub_sat(min_int + 5, 5), min_int);
    EXPECT_EQ(sub_sat(max_int - 5, -5), max_int);
    EXPECT_EQ(sub_sat(min_size_t + size_t{5}, size_t{5}), min_size_t);
}

TEST(SaturationArithmeticTest, SubSatOverflow) {
    EXPECT_EQ(sub_sat(min_int, 1), min_int);
    EXPECT_EQ(sub_sat(min_size_t, size_t{1}), min_size_t);
}

TEST(SaturationArithmeticTest, AbsSat) {
    // This is the only edge case where `abs_sat()` doesn't behave like `std:abs()`.
    EXPECT_EQ(abs_sat(std::numeric_limits<int8_t>::min()), std::numeric_limits<int8_t>::max());
    EXPECT_EQ(abs_sat(std::numeric_limits<int>::min()), std::numeric_limits<int>::max());
}

}  // namespace base
