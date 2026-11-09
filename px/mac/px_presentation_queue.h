#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <utility>

// Bounds outstanding frame submissions and remembers the latest deferred paint request.
// The layer serializes access between painting and Metal's presentation callbacks.
class px_presentation_queue {
public:
    static constexpr int kDrawableCount = 3;

    // Use the screen's shortest supported interval as the full-speed reference.
    // Old-screen callbacks still release reservations, but cannot train the new screen.
    void configure(uint32_t display, double refresh_interval) {
        if (!std::isfinite(refresh_interval) || refresh_interval <= 0.0) {
            refresh_interval = 1.0 / 60.0;
        }
        if (display != display_ || refresh_interval != refresh_interval_) {
            display_ = display;
            refresh_interval_ = refresh_interval;
            ++generation_;
            reset_observations();
        }
    }

    uint64_t generation() const { return generation_; }

    // Resizing must join the current Core Animation transaction, even with a full queue.
    bool try_submit(uint64_t request_id, bool resizing = false) {
        if (!resizing && pending_ >= pending_limit_) {
            deferred_request_ = request_id;
            backpressured_ = true;
            return false;
        }
        ++pending_;
        deferred_request_ = 0;
        return true;
    }

    // Complete each reservation after presentation, or after GPU completion if no drawable
    // was acquired. A nonzero result requests a repaint if that request is still the latest.
    uint64_t completed(double presented_time = 0.0, uint64_t generation = 0) {
        assert(pending_ > 0);
        --pending_;
        if (generation == generation_ && std::isfinite(presented_time) &&
            presented_time > last_presented_) {
            if (last_presented_ > 0.0) {
                intervals_[sample_count_++] = presented_time - last_presented_;
                if (sample_count_ == intervals_.size()) {
                    std::sort(intervals_.begin(), intervals_.end());
                    // The lower quartile ignores isolated missed refreshes. Only actual
                    // backpressure warrants reducing the queue; sparse input alone does not.
                    // The boundary lies halfway between one refresh and two. Two outstanding
                    // frames still allow adjacent refreshes, so faster delivery can restore three.
                    bool slower = intervals_[intervals_.size() / 4 - 1] > 1.5 * refresh_interval_;
                    pending_limit_ = backpressured_ && slower ? 2 : kDrawableCount;
                    sample_count_ = 0;
                    backpressured_ = false;
                }
            }
            last_presented_ = presented_time;
        }
        if (pending_ == 0) {
            reset_observations();
        }
        return pending_ < pending_limit_ ? std::exchange(deferred_request_, 0) : 0;
    }

private:
    void reset_observations() {
        pending_limit_ = kDrawableCount;
        last_presented_ = 0.0;
        sample_count_ = 0;
        backpressured_ = false;
    }

    int pending_ = 0;
    int pending_limit_ = kDrawableCount;
    uint64_t deferred_request_ = 0;
    uint32_t display_ = 0;
    uint64_t generation_ = 0;
    double refresh_interval_ = 1.0 / 60.0;
    double last_presented_ = 0.0;
    // Eight samples require sustained slower delivery before reducing queue depth.
    std::array<double, 8> intervals_{};
    size_t sample_count_ = 0;
    bool backpressured_ = false;
};
