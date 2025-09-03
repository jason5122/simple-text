#include "base/numeric/safe_conversions.h"
#include <gtest/gtest.h>

namespace base {

TEST(SafeConversionsTest, CheckedCast) {
    EXPECT_EQ(checked_cast<size_t>(1), 1);
    EXPECT_EQ(checked_cast<int8_t>(-128), -128);
}

TEST(SafeConversionsDeathTest, CheckedCastNegativeToUnsigned) {
    EXPECT_DEATH(checked_cast<size_t>(-1), "");
    EXPECT_DEATH(checked_cast<uint32_t>(-1), "");
}

TEST(SafeConversionsDeathTest, CheckedCastTooLarge) {
    EXPECT_DEATH(checked_cast<uint8_t>(256), "");
    EXPECT_DEATH(checked_cast<uint16_t>(1u << 20), "");
    EXPECT_DEATH(checked_cast<int8_t>(128), "");
    EXPECT_DEATH(checked_cast<int32_t>(std::numeric_limits<uint32_t>::max()), "");
}

TEST(SafeConversionsDeathTest, CheckedCastTooSmall) {
    EXPECT_DEATH(checked_cast<int8_t>(-129), "");
}

TEST(SafeConversionsTest, IsValueNegative) {
    EXPECT_TRUE(is_value_negative(-1));
    EXPECT_FALSE(is_value_negative(0));
    EXPECT_FALSE(is_value_negative(1));

    EXPECT_TRUE(is_value_negative(-1.0));
    EXPECT_FALSE(is_value_negative(0.0));
    EXPECT_FALSE(is_value_negative(1.0));
}

TEST(SafeConversionsTest, SafeUnsignedAbs) {
    EXPECT_EQ(safe_unsigned_abs(-1), 1);
    EXPECT_EQ(safe_unsigned_abs(0), 0);
    EXPECT_EQ(safe_unsigned_abs(1), 1);
}

}  // namespace base
