#include "ui/scroll_predictor.h"
#include <algorithm>

namespace {

constexpr double kTrajectoryResetSeconds = 0.100;
constexpr double kMinimumInputIntervalSeconds = 0.002;
constexpr double kMaximumPredictionSeconds = 0.008;
constexpr double kFitWindowSeconds = 0.024;
// How far a late event may be pulled back toward the cadence. Delivery jitter is a few
// milliseconds; anything later is a real pause or a slower device and is kept as it came.
constexpr double kMaximumSnapSeconds = 0.004;

}  // namespace

void scroll_predictor::reset() {
    points_ = {};
    next_ = 0;
    count_ = 0;
    intervals_ = {};
    interval_count_ = 0;
    latest_raw_timestamp_ = 0.0;
}

double scroll_predictor::cadence() const {
    if (interval_count_ < 2) {
        return 0.0;
    }
    std::array<double, kIntervalCount> sorted = intervals_;
    std::sort(sorted.begin(), sorted.begin() + static_cast<std::ptrdiff_t>(interval_count_));
    return sorted[interval_count_ / 2];
}

void scroll_predictor::update(double position, double timestamp) {
    if (count_ != 0 && timestamp - latest_raw_timestamp_ > kTrajectoryResetSeconds) {
        reset();
    }

    if (count_ != 0 && timestamp <= latest_raw_timestamp_) {
        // Several native events can share a timestamp. Preserve the newest cumulative position
        // without manufacturing a zero-length velocity.
        points_[(next_ + kCapacity - 1) % kCapacity].position = position;
        return;
    }

    double corrected = timestamp;
    if (count_ != 0) {
        const double raw_interval = timestamp - latest_raw_timestamp_;
        const double period = cadence();
        if (period > 0.0) {
            const double expected = latest().timestamp + period;
            if (expected < timestamp) {
                corrected =
                    std::max(expected, timestamp - std::min(kMaximumSnapSeconds, period * 0.5));
            }
            corrected = std::max(corrected, latest().timestamp + 0.000001);
        }
        if (interval_count_ < kIntervalCount) {
            intervals_[interval_count_++] = raw_interval;
        } else {
            std::rotate(intervals_.begin(), intervals_.begin() + 1, intervals_.end());
            intervals_[kIntervalCount - 1] = raw_interval;
        }
    }
    latest_raw_timestamp_ = timestamp;

    points_[next_] = {position, corrected};
    next_ = (next_ + 1) % kCapacity;
    count_ = std::min(count_ + 1, kCapacity);
}

double scroll_predictor::sample(double sample_time) const {
    if (count_ == 0) {
        return 0.0;
    }
    if (count_ == 1) {
        return latest().position;
    }

    // The events inside the window, or the last two when the stream is slower than the window
    // (a momentum tail drops to 30 Hz).
    size_t used = 2;
    while (used < count_ &&
           latest().timestamp - from_latest(used).timestamp <= kFitWindowSeconds) {
        ++used;
    }

    const double span = latest().timestamp - from_latest(used - 1).timestamp;
    if (span < kMinimumInputIntervalSeconds) {
        return latest().position;
    }
    const double input_interval = span / static_cast<double>(used - 1);

    double mean_time = 0.0;
    double mean_position = 0.0;
    for (size_t back = 0; back < used; ++back) {
        mean_time += from_latest(back).timestamp;
        mean_position += from_latest(back).position;
    }
    mean_time /= static_cast<double>(used);
    mean_position /= static_cast<double>(used);

    double covariance = 0.0;
    double variance = 0.0;
    for (size_t back = 0; back < used; ++back) {
        const double dt = from_latest(back).timestamp - mean_time;
        covariance += dt * (from_latest(back).position - mean_position);
        variance += dt * dt;
    }
    const double velocity = covariance / variance;

    const double maximum_prediction = std::min(kMaximumPredictionSeconds, input_interval);
    sample_time = std::clamp(sample_time, from_latest(used - 1).timestamp,
                             latest().timestamp + maximum_prediction);
    return mean_position + velocity * (sample_time - mean_time);
}
