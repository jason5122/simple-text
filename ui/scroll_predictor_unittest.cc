#include "ui/scroll_predictor.h"
#include <gtest/gtest.h>
#include <vector>

TEST(ScrollPredictor, ReturnsLatestPositionUntilItHasVelocity) {
    scroll_predictor predictor;
    EXPECT_DOUBLE_EQ(predictor.sample(1.0), 0.0);
    predictor.update(42.0, 1.0);
    EXPECT_DOUBLE_EQ(predictor.sample(1.1), 42.0);
}

TEST(ScrollPredictor, InterpolatesBetweenTwoInputs) {
    scroll_predictor predictor;
    predictor.update(0.0, 1.000);
    predictor.update(10.0, 1.010);
    EXPECT_NEAR(predictor.sample(1.0025), 2.5, 0.001);
    EXPECT_NEAR(predictor.sample(1.010), 10.0, 0.001);
}

TEST(ScrollPredictor, NeverSamplesBeforeTheOldestFittedInput) {
    scroll_predictor predictor;
    predictor.update(0.0, 1.000);
    predictor.update(10.0, 1.010);
    EXPECT_NEAR(predictor.sample(0.5), 0.0, 0.001);
}

TEST(ScrollPredictor, CapsForwardPredictionAtOneInputInterval) {
    scroll_predictor predictor;
    predictor.update(0.0, 1.000);
    predictor.update(10.0, 1.005);

    // However far away the requested time is, five milliseconds of input can extrapolate only
    // five milliseconds, producing 20 rather than an unbounded position.
    EXPECT_NEAR(predictor.sample(2.0), 20.0, 0.001);
}

TEST(ScrollPredictor, CapsForwardPredictionAtEightMilliseconds) {
    scroll_predictor predictor;
    predictor.update(0.0, 1.000);
    predictor.update(100.0, 1.050);
    EXPECT_NEAR(predictor.sample(2.0), 116.0, 0.001);
}

TEST(ScrollPredictor, ResetsVelocityAfterAnInputGap) {
    scroll_predictor predictor;
    predictor.update(0.0, 1.0);
    predictor.update(10.0, 1.01);
    predictor.update(20.0, 1.2);
    EXPECT_DOUBLE_EQ(predictor.sample(1.3), 20.0);
}

TEST(ScrollPredictor, KeepsVelocityWhenEventsShareATimestamp) {
    scroll_predictor predictor;
    predictor.update(0.0, 1.000);
    predictor.update(10.0, 1.010);
    predictor.update(12.0, 1.010);
    EXPECT_NEAR(predictor.sample(1.005), 6.0, 0.001);
}

TEST(ScrollPredictor, SmoothsUnevenlySplitEventPairs) {
    // A trackpad drag at 240 Hz whose 8 ms of motion (24 points) arrives as 3 then 21, sampled
    // once per 120 Hz frame at a phase that drifts through the pairs.
    scroll_predictor predictor;
    double position = 0.0;
    double time = 1.0;
    std::vector<double> steps;
    double previous = 0.0;
    for (int i = 0; i < 40; ++i) {
        time += 1.0 / 240.0;
        position += i % 2 == 0 ? 3.0 : 21.0;
        predictor.update(position, time);
        if (i >= 8 && i % 2 == 1) {
            const double sampled = predictor.sample(time - 0.0095 + 0.0007 * (i / 2));
            if (i > 9) {
                steps.push_back(sampled - previous);
            }
            previous = sampled;
        }
    }
    // Every frame moves close to the 24 points per frame the pairs add up to.
    for (double step : steps) {
        EXPECT_NEAR(step, 24.0, 4.0);
    }
}

TEST(ScrollPredictor, PullsALateTimestampBackToTheCadence) {
    // Momentum events at 120 Hz carrying 40 points each, one of them delivered 3 ms late. The
    // trajectory through the corrected timestamps stays a straight 4.8 points per millisecond.
    scroll_predictor predictor;
    predictor.update(0.0, 1.0000);
    predictor.update(40.0, 1.00833);
    predictor.update(80.0, 1.01667);
    predictor.update(120.0, 1.02800);  // 3 ms late
    predictor.update(160.0, 1.03333);
    EXPECT_NEAR(predictor.sample(1.0300), 144.0, 2.0);
}

TEST(ScrollPredictor, KeepsAGenuinePause) {
    // A 30 ms hole is a pause, not jitter: the event after it keeps (nearly) its own time.
    scroll_predictor predictor;
    predictor.update(0.0, 1.0000);
    predictor.update(40.0, 1.00833);
    predictor.update(80.0, 1.01667);
    predictor.update(120.0, 1.04667);
    EXPECT_NEAR(predictor.latest_timestamp(), 1.04667, 0.0041);
    EXPECT_GE(predictor.latest_timestamp(), 1.0426);
}

TEST(ScrollPredictor, FitsOnlyTheRecentWindow) {
    // Motion that stopped: once the moving events fall outside the 24 ms window the fit is flat.
    scroll_predictor predictor;
    predictor.update(0.0, 1.000);
    predictor.update(50.0, 1.008);
    predictor.update(100.0, 1.016);
    predictor.update(100.0, 1.050);
    predictor.update(100.0, 1.058);
    predictor.update(100.0, 1.066);
    EXPECT_NEAR(predictor.sample(1.062), 100.0, 0.001);
}

TEST(ScrollPredictor, ReportsTheStreamCadenceOnceKnown) {
    scroll_predictor predictor;
    predictor.update(0.0, 1.000);
    predictor.update(10.0, 1.008);
    EXPECT_DOUBLE_EQ(predictor.cadence(), 0.0);
    predictor.update(20.0, 1.016);
    predictor.update(30.0, 1.024);
    EXPECT_NEAR(predictor.cadence(), 0.008, 0.0001);
}
