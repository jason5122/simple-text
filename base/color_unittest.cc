#include "base/color.h"
#include <gtest/gtest.h>

TEST(ColorTest, ConvertsNormalisedChannelsToPackedRgbaStorage) {
    const color value = color::from_normalised(1.0f, 128.0f / 255.0f, 0.0f, 64.0f / 255.0f);

    EXPECT_EQ(value.red(), 255);
    EXPECT_EQ(value.green(), 128);
    EXPECT_EQ(value.blue(), 0);
    EXPECT_EQ(value.alpha(), 64);
    EXPECT_EQ(value.packed_argb(), 0x40ff8000u);
}

TEST(ColorTest, RoundsNormalisedChannelsToTheNearestByte) {
    const color value = color::from_normalised(0.0f, 0.5f, 1.0f, 0.25f);

    EXPECT_EQ(value.red(), 0);
    EXPECT_EQ(value.green(), 128);
    EXPECT_EQ(value.blue(), 255);
    EXPECT_EQ(value.alpha(), 64);
}

TEST(FColorTest, ConvertsToAndFromPackedArgb) {
    const fcolor value = fcolor::from_packed_argb(0x4080ff00u);

    EXPECT_FLOAT_EQ(value.r, 128.0f / 255.0f);
    EXPECT_FLOAT_EQ(value.g, 1.0f);
    EXPECT_FLOAT_EQ(value.b, 0.0f);
    EXPECT_FLOAT_EQ(value.a, 64.0f / 255.0f);
    EXPECT_EQ(value.packed_argb(), 0x4080ff00u);
}
