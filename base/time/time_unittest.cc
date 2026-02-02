#include "base/time/time.h"
#include <gtest/gtest.h>

namespace base {

TEST(TimeDeltaTest, Operators) {
    auto u10 = TimeDelta::microseconds(10);
    auto u20 = TimeDelta::microseconds(20);
    auto u50 = TimeDelta::microseconds(50);

    EXPECT_EQ(u10 + u10, u20);
    EXPECT_EQ(u10 + u20 + u20, u50);
    EXPECT_EQ(u10 - u10, TimeDelta());

    auto t1 = u10;
    t1 += u10;
    EXPECT_EQ(t1, u20);

    auto t2 = u10;
    t2 -= u10;
    EXPECT_EQ(t2, TimeDelta());

    EXPECT_EQ(u10 * 5, u50);
    EXPECT_EQ(u50 / 5, u10);
}

TEST(TimeTicksTest, Operators) {
    auto u10 = TimeDelta::microseconds(10);
    auto u20 = TimeDelta::microseconds(20);
    auto t10 = TimeTicks() + u10;
    auto t20 = TimeTicks() + u20;

    EXPECT_EQ(TimeTicks() + u10, t10);
    EXPECT_EQ(t10 - u10, TimeTicks());

    auto plus = TimeTicks();
    plus += u10;
    EXPECT_EQ(plus, t10);

    auto minus = t10;
    minus -= u10;
    EXPECT_EQ(minus, TimeTicks());
    minus -= u20;
    EXPECT_EQ(minus, TimeTicks() - u20);

    EXPECT_EQ(t20 - t10, u10);
    EXPECT_EQ(t10 - t20, -u10);
}

}  // namespace base
