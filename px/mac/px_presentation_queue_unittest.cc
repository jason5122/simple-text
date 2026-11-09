#include "px/mac/px_presentation_queue.h"
#include <deque>
#include <gtest/gtest.h>

namespace {

// A display only completes admitted frames. This closes the feedback loop that
// previously let slow input permanently limit subsequent full-rate scrolling.
class presentation_simulator {
public:
    explicit presentation_simulator(int pipeline_delay = 2, double hz = 120.0)
        : pipeline_delay_(pipeline_delay), refresh_interval_(1.0 / hz) {
        queue_.configure(1, refresh_interval_);
    }

    int paint_for(int ticks, int input_period = 1, int display_period = 1) {
        int presented = 0;
        maximum_delay_ = 0;
        for (int tick = 0; tick < ticks; ++tick, ++tick_) {
            if (tick_ % display_period == 0 && !pending_.empty() &&
                tick_ - pending_.front() >= pipeline_delay_) {
                maximum_delay_ = std::max(maximum_delay_, tick_ - pending_.front());
                queue_.completed(10.0 + tick_ * refresh_interval_, queue_.generation());
                pending_.pop_front();
                ++presented;
            }
            if (tick_ % input_period == 0 && queue_.try_submit(++frame_id_)) {
                pending_.push_back(tick_);
            }
        }
        return presented;
    }

    int maximum_delay() const { return maximum_delay_; }

private:
    px_presentation_queue queue_;
    std::deque<int> pending_;
    int tick_ = 0;
    int pipeline_delay_;
    int maximum_delay_ = 0;
    double refresh_interval_;
    uint64_t frame_id_ = 0;
};

TEST(PresentationQueueTest, CoalescesToLatestRequestUntilPresentationCompletes) {
    px_presentation_queue queue;
    EXPECT_TRUE(queue.try_submit(1));
    EXPECT_TRUE(queue.try_submit(2));
    EXPECT_TRUE(queue.try_submit(3));
    EXPECT_FALSE(queue.try_submit(4));
    EXPECT_FALSE(queue.try_submit(5));
    EXPECT_EQ(queue.completed(), 5u);
    EXPECT_EQ(queue.completed(), 0u);
    EXPECT_TRUE(queue.try_submit(6));
}

TEST(PresentationQueueTest, NewSubmissionSupersedesDeferredRequest) {
    px_presentation_queue queue;
    queue.try_submit(1);
    queue.try_submit(2);
    queue.try_submit(3);
    EXPECT_FALSE(queue.try_submit(4));
    EXPECT_TRUE(queue.try_submit(5, /*resizing=*/true));
    EXPECT_EQ(queue.completed(), 0u);
    EXPECT_EQ(queue.completed(), 0u);
}

TEST(PresentationQueueTest, ResizeReservationsRemainCountedUntilCompletion) {
    px_presentation_queue queue;
    queue.try_submit(1);
    queue.try_submit(2);
    queue.try_submit(3);
    EXPECT_TRUE(queue.try_submit(4, /*resizing=*/true));
    EXPECT_FALSE(queue.try_submit(5));
    EXPECT_EQ(queue.completed(), 0u);
    EXPECT_FALSE(queue.try_submit(6));
    EXPECT_EQ(queue.completed(), 6u);
    EXPECT_TRUE(queue.try_submit(7));
    EXPECT_FALSE(queue.try_submit(8));
}

TEST(PresentationQueueTest, FailedAcquisitionOrDroppedFrameReleasesReservation) {
    px_presentation_queue queue;
    queue.try_submit(1);
    queue.try_submit(2);
    queue.try_submit(3);
    EXPECT_FALSE(queue.try_submit(4));
    EXPECT_EQ(queue.completed(), 4u);
    EXPECT_TRUE(queue.try_submit(5));
    EXPECT_FALSE(queue.try_submit(6));
}

TEST(PresentationQueueTest, RecoversFullRateAfterSlowInput) {
    // Tick counts make this independent of the display's absolute refresh rate.
    for (int pipeline_delay : {2, 3}) {
        for (int input_period : {2, 4, 16}) {
            SCOPED_TRACE(pipeline_delay);
            SCOPED_TRACE(input_period);
            presentation_simulator display(pipeline_delay);
            display.paint_for(240, input_period);
            display.paint_for(120);
            EXPECT_EQ(display.paint_for(120), 120);
        }
    }
}

TEST(PresentationQueueTest, SustainsFullRateWithThreeFramesInThePipeline) {
    presentation_simulator display(3);
    display.paint_for(120);
    EXPECT_EQ(display.paint_for(120), 120);
}

TEST(PresentationQueueTest, BoundsLatencyWhenPresentationSlows) {
    for (double hz : {60.0, 120.0, 144.0, 240.0}) {
        SCOPED_TRACE(hz);
        presentation_simulator display(2, hz);
        display.paint_for(120, 1, 2);
        EXPECT_EQ(display.paint_for(120, 1, 2), 60);
        EXPECT_LE(display.maximum_delay(), 4);
    }
}

TEST(PresentationQueueTest, RecoversFullRateWithDeepPipelineAfterSlowPresentation) {
    for (double hz : {60.0, 120.0, 144.0, 240.0}) {
        SCOPED_TRACE(hz);
        presentation_simulator display(3, hz);
        display.paint_for(240, 1, 2);
        display.paint_for(120);
        EXPECT_EQ(display.paint_for(120), 120);
    }
}

TEST(PresentationQueueTest, OldScreenCompletionsReleaseTheirReservations) {
    px_presentation_queue queue;
    queue.configure(1, 1.0 / 120.0);
    uint64_t old_generation = queue.generation();
    queue.try_submit(1);
    queue.try_submit(2);
    queue.try_submit(3);
    EXPECT_FALSE(queue.try_submit(4));
    queue.configure(2, 1.0 / 60.0);
    EXPECT_EQ(queue.completed(10.0, old_generation), 4u);
    EXPECT_TRUE(queue.try_submit(5));
}

TEST(PresentationQueueTest, FollowsDisplayCadenceAndRecoversWhenItSpeedsUp) {
    presentation_simulator display;
    display.paint_for(120);
    EXPECT_EQ(display.paint_for(120), 120);
    display.paint_for(120, 1, 2);
    EXPECT_EQ(display.paint_for(120, 1, 2), 60);
    display.paint_for(120, 1, 3);
    EXPECT_EQ(display.paint_for(120, 1, 3), 40);
    display.paint_for(120);
    EXPECT_EQ(display.paint_for(120), 120);
}

TEST(PresentationQueueTest, FinalDeferredPaintGetsWakeupWithoutFurtherInput) {
    px_presentation_queue queue;
    queue.try_submit(1);
    queue.try_submit(2);
    queue.try_submit(3);
    EXPECT_FALSE(queue.try_submit(4));
    EXPECT_EQ(queue.completed(), 4u);
    EXPECT_TRUE(queue.try_submit(5));
    EXPECT_EQ(queue.completed(), 0u);
    EXPECT_EQ(queue.completed(), 0u);
    EXPECT_EQ(queue.completed(), 0u);
}

}  // namespace
