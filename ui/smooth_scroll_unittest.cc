#include "ui/smooth_scroll.h"
#include <gtest/gtest.h>
#include <vector>

namespace {

constexpr double kInterval = 1.0 / 120.0;
constexpr double kNoLimit = 1e9;

// Ticks are offset from the events by a fraction of an interval, as they are in practice.
constexpr double kTickPhase = 0.003;

}  // namespace

TEST(SmoothScroll, StartsAGestureOnTheFirstEvent) {
    smooth_scroll scroll;
    EXPECT_FALSE(scroll.animating());
    EXPECT_TRUE(scroll.scroll(0.0, 1.0, kNoLimit));
    EXPECT_TRUE(scroll.animating());
    EXPECT_FALSE(scroll.scroll(10.0, 1.0 + kInterval, kNoLimit));
}

TEST(SmoothScroll, MovesTheSameDistanceEveryFrameOnASteadyStream) {
    smooth_scroll scroll;
    double time = 1.0;
    scroll.scroll(0.0, time, kNoLimit);
    std::vector<double> steps;
    double previous = scroll.offset();
    for (int i = 0; i < 24; ++i) {
        time += kInterval;
        scroll.scroll(12.0, time, kNoLimit);
        scroll.tick(time + kTickPhase, kNoLimit);
        if (i >= 4) {
            steps.push_back(scroll.offset() - previous);
        }
        previous = scroll.offset();
    }
    for (double step : steps) {
        EXPECT_NEAR(step, 12.0, 0.5);
    }
}

TEST(SmoothScroll, KeepsMovingThroughATickThatSawNoNewEvent) {
    // Input at 120 Hz drifting against ticks at 120 Hz leaves the odd tick without a new event.
    smooth_scroll scroll;
    double time = 1.0;
    scroll.scroll(0.0, time, kNoLimit);
    for (int i = 0; i < 8; ++i) {
        time += kInterval;
        scroll.scroll(12.0, time, kNoLimit);
        scroll.tick(time + kTickPhase, kNoLimit);
    }
    const double before = scroll.offset();
    EXPECT_TRUE(scroll.tick(time + kTickPhase + kInterval, kNoLimit));
    EXPECT_NEAR(scroll.offset() - before, 12.0, 1.0);
}

TEST(SmoothScroll, NeverMovesBackwardWhileTheInputMovesForward) {
    smooth_scroll scroll;
    double time = 1.0;
    scroll.scroll(0.0, time, kNoLimit);
    double previous = 0.0;
    for (int i = 0; i < 30; ++i) {
        // Uneven pairs at 240 Hz, as a trackpad delivers them.
        time += kInterval / 2.0;
        scroll.scroll(i % 2 == 0 ? 3.0 : 21.0, time, kNoLimit);
        if (i % 2 == 1) {
            scroll.tick(time + kTickPhase, kNoLimit);
            EXPECT_GE(scroll.offset(), previous);
            previous = scroll.offset();
        }
    }
}

TEST(SmoothScroll, RestsWhereTheTrajectoryEndedWhenInputStops) {
    smooth_scroll scroll;
    double time = 1.0;
    scroll.scroll(0.0, time, kNoLimit);
    for (int i = 0; i < 12; ++i) {
        time += kInterval;
        scroll.scroll(30.0, time, kNoLimit);
        scroll.tick(time + kTickPhase, kNoLimit);
    }
    const double last_input = time;
    double previous = scroll.offset();
    double resting = 0.0;
    for (int i = 1; i <= 12; ++i) {
        scroll.tick(last_input + kTickPhase + i * kInterval, kNoLimit);
        EXPECT_GE(scroll.offset(), previous);
        previous = scroll.offset();
        if (i == 8) {
            resting = scroll.offset();
        }
    }
    EXPECT_DOUBLE_EQ(scroll.offset(), resting);
    EXPECT_LE(scroll.offset(), 12 * 30.0 + 30.0);  // at most a frame of motion past the finger
}

TEST(SmoothScroll, ContinuesForwardWhenScrollingResumesAfterAPause) {
    smooth_scroll scroll;
    double time = 1.0;
    scroll.scroll(0.0, time, kNoLimit);
    for (int i = 0; i < 12; ++i) {
        time += kInterval;
        scroll.scroll(30.0, time, kNoLimit);
        scroll.tick(time + kTickPhase, kNoLimit);
    }
    scroll.tick(time + kTickPhase + kInterval, kNoLimit);
    scroll.tick(time + kTickPhase + 2 * kInterval, kNoLimit);
    const double before_pause = scroll.offset();

    time += 0.040;
    scroll.scroll(30.0, time, kNoLimit);
    time += kInterval;
    scroll.scroll(30.0, time, kNoLimit);
    scroll.tick(time + kTickPhase, kNoLimit);
    EXPECT_GT(scroll.offset(), before_pause);
    EXPECT_LT(scroll.offset(), before_pause + 60.0);
}

TEST(SmoothScroll, StopsAnimatingAfterTwentyIdleTicks) {
    smooth_scroll scroll;
    scroll.scroll(0.0, 1.0, kNoLimit);
    scroll.scroll(10.0, 1.0 + kInterval, kNoLimit);
    double time = 1.0 + kInterval;
    for (int i = 0; i < 19; ++i) {
        time += kInterval;
        scroll.tick(time + kTickPhase, kNoLimit);
    }
    // The first ticks still move toward the input; only ticks that changed nothing count.
    EXPECT_TRUE(scroll.animating());
    for (int i = 0; i < 20; ++i) {
        time += kInterval;
        scroll.tick(time + kTickPhase, kNoLimit);
    }
    EXPECT_FALSE(scroll.animating());
}

TEST(SmoothScroll, JumpToEndsTheGesture) {
    smooth_scroll scroll;
    double time = 1.0;
    scroll.scroll(0.0, time, kNoLimit);
    for (int i = 0; i < 6; ++i) {
        time += kInterval;
        scroll.scroll(30.0, time, kNoLimit);
        scroll.tick(time + kTickPhase, kNoLimit);
    }
    scroll.jump_to(500.0, kNoLimit);
    EXPECT_DOUBLE_EQ(scroll.offset(), 500.0);
    EXPECT_FALSE(scroll.animating());
    EXPECT_FALSE(scroll.tick(time + kTickPhase + kInterval, kNoLimit));
    EXPECT_DOUBLE_EQ(scroll.offset(), 500.0);
}

TEST(SmoothScroll, StaysWithinTheRange) {
    smooth_scroll scroll;
    scroll.jump_to(-5.0, 100.0);
    EXPECT_DOUBLE_EQ(scroll.offset(), 0.0);
    double time = 1.0;
    scroll.scroll(0.0, time, 100.0);
    for (int i = 0; i < 20; ++i) {
        time += kInterval;
        scroll.scroll(30.0, time, 100.0);
        scroll.tick(time + kTickPhase, 100.0);
        EXPECT_LE(scroll.offset(), 100.0);
    }
    EXPECT_DOUBLE_EQ(scroll.offset(), 100.0);
}

TEST(SmoothScroll, IgnoresZeroDeltaEvents) {
    smooth_scroll scroll;
    double time = 1.0;
    scroll.scroll(0.0, time, kNoLimit);
    for (int i = 0; i < 6; ++i) {
        time += kInterval;
        scroll.scroll(0.0, time, kNoLimit);
        EXPECT_FALSE(scroll.tick(time + kTickPhase, kNoLimit));
    }
    EXPECT_DOUBLE_EQ(scroll.offset(), 0.0);
}
