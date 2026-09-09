#pragma once

#include "ui/scroll_predictor.h"

// Chromium-style smooth scrolling for one axis. Precise scroll events (trackpad, Magic Mouse)
// feed a trajectory; a display-clock tick chooses the offset to show, sampled a little behind the
// tick so that it interpolates between real events and moves the same distance every frame even
// though the input and display clocks drift against each other. Events never move the offset
// themselves, so there is exactly one frame per refresh while a gesture runs.
//
// Timestamps are seconds on one monotonic clock (px_now()): the event's own time for input, the
// tick's time for tick(). Offsets are in the caller's units, 0 to `maximum`, growing as the
// content scrolls forward.
class smooth_scroll {
public:
    double offset() const { return offset_; }

    // Puts the content at `offset` right away (scrollbar, keyboard, a line-based wheel) and ends
    // any gesture in progress.
    void jump_to(double offset, double maximum);

    // A precise scroll event, `delta` in offset units. Returns true when this starts a gesture:
    // the caller must begin delivering ticks and should draw a frame now, unchanged as it is,
    // because the window server leaves its idle refresh rate on the first commit and macOS sends
    // the touch before the motion.
    bool scroll(double delta, double timestamp, double maximum);

    // Display-clock tick. Returns true when offset() changed. animating() turns false once input
    // has been idle for a second, after which the caller can stop the ticks.
    bool tick(double now, double maximum);
    bool animating() const { return animating_; }

private:
    scroll_predictor predictor_;
    double offset_ = 0.0;
    double target_ = 0.0;  // accumulated input, clamped to the range
    double last_input_time_ = 0.0;
    double last_delta_ = 0.0;
    bool animating_ = false;
    bool settled_ = false;
};
