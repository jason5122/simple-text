#include "base/geometry.h"

#include <gtest/gtest.h>

static_assert(sizeof(vec2) == 16);
static_assert(sizeof(vec2i) == 8);
static_assert(sizeof(rect) == 32);
static_assert(sizeof(recti) == 16);

TEST(Vec2Test, ValueInitializationProducesTheOrigin) {
    const vec2 point;

    EXPECT_DOUBLE_EQ(point.x, 0.0);
    EXPECT_DOUBLE_EQ(point.y, 0.0);
}

TEST(Vec2iTest, ValueInitializationProducesTheOrigin) {
    const vec2i point;

    EXPECT_EQ(point.x, 0);
    EXPECT_EQ(point.y, 0);
}

TEST(RectTest, DerivesRightAndBottomEdgesFromItsExtent) {
    const rect area{10.5, 20.25, 30.75, 40.5};

    EXPECT_DOUBLE_EQ(area.right(), 41.25);
    EXPECT_DOUBLE_EQ(area.bottom(), 60.75);
}

TEST(RectTest, NonPositiveExtentIsEmpty) {
    EXPECT_FALSE((rect{0.0, 0.0, 1.0, 1.0}).empty());
    EXPECT_TRUE((rect{0.0, 0.0, 0.0, 1.0}).empty());
    EXPECT_TRUE((rect{0.0, 0.0, 1.0, 0.0}).empty());
    EXPECT_TRUE((rect{0.0, 0.0, -1.0, 1.0}).empty());
    EXPECT_TRUE((rect{0.0, 0.0, 1.0, -1.0}).empty());
}

TEST(RectiTest, DerivesExtentFromItsEdges) {
    const recti area{10, 20, 41, 61};

    EXPECT_EQ(area.width(), 31);
    EXPECT_EQ(area.height(), 41);
}

TEST(RectiTest, NonIncreasingEdgesAreEmpty) {
    EXPECT_FALSE((recti{0, 0, 1, 1}).empty());
    EXPECT_TRUE((recti{1, 0, 1, 1}).empty());
    EXPECT_TRUE((recti{0, 1, 1, 1}).empty());
    EXPECT_TRUE((recti{2, 0, 1, 1}).empty());
    EXPECT_TRUE((recti{0, 2, 1, 1}).empty());
}
