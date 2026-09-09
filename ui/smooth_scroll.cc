#include "ui/smooth_scroll.h"
#include <algorithm>

namespace {

// How far behind the tick the trajectory is sampled: one 120 Hz input interval plus delivery
// jitter, so the sample normally falls between two real events. A tick whose interval brought no
// event then still advances along the line instead of repeating a frame and doubling up on the
// next one. Chromium's LinearResampling samples 5 ms behind its vsync-aligned frame time for the
// same reason.
constexpr double kResampleLatencySeconds = 0.0095;

// Input this long after the previous event is a pause or a new gesture, whichever is longer of a
// fixed floor and a multiple of the stream's own cadence: a momentum tail slows to 33 ms
// intervals and must not count as pausing.
constexpr double kPauseSeconds = 0.025;
constexpr double kPauseCadenceMultiple = 2.5;

// Seeds a new trajectory this far in the past when the stream's cadence is not known yet, so the
// first delta carries a velocity instead of a jump.
constexpr double kSeedIntervalSeconds = 1.0 / 120.0;

constexpr double kSettleSeconds = 0.050;

// Ticks with nothing to do before the display clock may stop; Chromium's kMaxKeepAliveCount.
constexpr int kKeepAliveTicks = 20;

}  // namespace

void smooth_scroll::jump_to(double offset, double maximum) {
    offset_ = std::clamp(offset, 0.0, maximum);
    target_ = offset_;
    last_delta_ = 0.0;
    animating_ = false;
    settled_ = false;
    predictor_.reset();
}

bool smooth_scroll::scroll(double delta, double timestamp, double maximum) {
    const double cadence = predictor_.cadence();
    const double pause = std::max(kPauseSeconds, kPauseCadenceMultiple * cadence);
    const bool starting = !animating_ || timestamp - last_input_time_ >= pause;
    if (starting) {
        // The trajectory restarts from where the content is. After a pause that can be a frame of
        // motion ahead of the accumulated input; continuing from there is invisible, whereas
        // standing still until the input caught up, or jumping back, is not.
        predictor_.reset();
        predictor_.update(offset_, timestamp - (cadence > 0.0 ? cadence : kSeedIntervalSeconds));
        target_ = offset_;
        last_delta_ = 0.0;
        animating_ = true;
    }
    settled_ = false;
    idle_ticks_ = 0;
    last_input_time_ = timestamp;

    // Phase markers (gesture began or ended, momentum ended) carry no motion. Fed to the
    // predictor they would flatten its last segment and stall a frame at the hand-off from
    // finger to momentum.
    if (delta == 0.0) {
        return starting;
    }
    target_ = std::clamp(target_ + delta, 0.0, maximum);
    last_delta_ = delta;
    predictor_.update(target_, timestamp);
    return starting;
}

bool smooth_scroll::tick(double now, double maximum) {
    if (!animating_) {
        return false;
    }
    const double previous = offset_;
    if (!sample_tick(now, maximum)) {
        offset_ = std::clamp(offset_, 0.0, maximum);
    }

    const bool changed = offset_ != previous;
    idle_ticks_ = changed ? 0 : idle_ticks_ + 1;
    if (idle_ticks_ >= kKeepAliveTicks) {
        animating_ = false;
    }
    return changed;
}

// Returns false once the gesture has settled and there is nothing left to sample.
bool smooth_scroll::sample_tick(double now, double maximum) {
    if (settled_) {
        return false;
    }
    double sampled = std::clamp(predictor_.sample(now - kResampleLatencySeconds), 0.0, maximum);
    // Chromium suppresses predicted deltas that oppose the latest real delta. Without this, a
    // small over-prediction shows up as a one-frame backward twitch.
    if (last_delta_ > 0.0) {
        sampled = std::max(sampled, offset_);
    } else if (last_delta_ < 0.0) {
        sampled = std::min(sampled, offset_);
    }
    offset_ = sampled;

    if (now - last_input_time_ >= kSettleSeconds) {
        // The finger stopped or lifted without momentum. Rest where the trajectory ended: it can
        // be ahead of the finger's final position by up to a frame of motion, which is invisible,
        // whereas snapping back to the accumulated input was a visible twitch.
        target_ = offset_;
        settled_ = true;
    }
    return true;
}
