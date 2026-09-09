#pragma once

#include <array>
#include <cstddef>

// A small, platform-neutral relative of Chromium's LinearResampling predictor: the position on
// the input trajectory at a display-chosen time, so that frames sample the finger at even
// intervals whatever the input clock does relative to the display's. Input positions remain
// authoritative; this class only chooses what to show in a particular frame.
//
// Chromium interpolates between the last two events. macOS's stream is not a clean trajectory at
// that scale: a trackpad drag arrives at 240 Hz as pairs with uneven deltas (3 then 21 points),
// and momentum events carry deltas for a nominal 120 Hz step while their timestamps arrive a few
// milliseconds late now and then. So this class first pulls a late timestamp back toward the
// stream's cadence, then fits a line through the events of the last 24 ms.
class scroll_predictor {
public:
    void reset();
    void update(double position, double timestamp);

    // Position on the fitted line at `sample_time`, no earlier than the oldest event in the fit
    // and no later than the latest event plus min(8 ms, one average input interval).
    double sample(double sample_time) const;

    bool has_input() const { return count_ != 0; }
    double latest_position() const { return latest().position; }
    double latest_timestamp() const { return latest().timestamp; }
    // The stream's typical interval (median of the last few), or zero until there are enough
    // events to know.
    double cadence() const;

private:
    struct sample_point {
        double position = 0.0;
        double timestamp = 0.0;  // after cadence correction
    };

    static constexpr size_t kCapacity = 10;
    static constexpr size_t kIntervalCount = 5;

    const sample_point& latest() const { return points_[(next_ + kCapacity - 1) % kCapacity]; }
    const sample_point& from_latest(size_t back) const {
        return points_[(next_ + kCapacity - 1 - back) % kCapacity];
    }

    std::array<sample_point, kCapacity> points_{};
    size_t next_ = 0;
    size_t count_ = 0;

    // Raw intervals between the last few events, for the cadence estimate.
    std::array<double, kIntervalCount> intervals_{};
    size_t interval_count_ = 0;
    double latest_raw_timestamp_ = 0.0;
};
