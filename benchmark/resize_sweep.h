// Resize-cadence measurement for resize_benchmark, kept apart from the px glue so another app can
// be measured by the same rules: the app owns the window and the clock; it feeds display ticks,
// resize events and paints in here, and this decides the sweep and prints one summary.
//
// Sweep mode: the window starts at its largest size and the sweep is a triangle wave below it,
// so the frame never leaves the area it first occupied and a persistent backing is never
// reallocated mid-run. Each display tick resizes once; Core Animation then paints the layer when
// it commits at the end of that run-loop turn, so a healthy step paints exactly one frame before
// the next tick and the interval between paints is one refresh.
//
// Drag mode: the steps are the window's own resize events. Steps only count while events keep
// arriving within 1.5 refreshes of each other, so hesitating mid-drag does not register as
// missed frames. Without display ticks the refresh is estimated from the event intervals.
//
// Readout: frames observed per step (one is right; none means the step was folded into the next
// one's frame, which the eye sees as one skipped size; two or more is a redundant frame), the wall
// time of each resize call, paint cost, resize-to-paint latency, gaps between consecutive paints
// in refresh units, and how evenly the display ticks themselves were delivered. A tick that
// arrives late is followed by one that arrives early, and a step issued on a late tick is the one
// that gets folded; a long run of late ticks is a blocked main thread. Compare runs on a quiet
// machine only: other processes contending for WindowServer show up here as tick jitter.

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace resize_sweep {

struct size {
    double w = 0.0;
    double h = 0.0;
};

struct options {
    int steps = 480;
    bool log = false;
    bool drag = false;
    bool dark = false;  // light text on a dark background; the scene is otherwise identical
    bool display_link = false;  // --drag only: keep the display link running during the drag
    size maximum{1600.0, 1000.0};
    size range{480.0, 300.0};
    size amplitude{24.0, 15.0};
};

// Ticks to let the first paint, activation and atlas uploads settle before the sweep starts.
constexpr int kSettleTicks = 30;
// A tick or paint interval this many refreshes long counts as late; a tick interval this short
// counts as early. Early is generous because a tick delivered a few milliseconds late is enough
// for Core Animation to hold the step's commit until the next turn, folding it into the next step.
constexpr double kLateRefreshes = 1.5;
constexpr double kEarlyRefreshes = 0.85;
// Used for the first few intervals of a drag before any estimate exists.
constexpr double kAssumedRefresh = 1.0 / 120.0;

inline bool parse_size(const char* text, size* out) {
    char* end = nullptr;
    const double w = std::strtod(text, &end);
    if (!end || *end != 'x' || w <= 0.0) return false;
    const double h = std::strtod(end + 1, &end);
    if (!end || *end != '\0' || h <= 0.0) return false;
    *out = size{w, h};
    return true;
}

inline void usage(const char* program) {
    std::fprintf(stderr,
                 "usage: %s [--steps N] [--size WxH] [--range WxH] [--amplitude WxH] [--log] "
                 "[--drag] [--dark] [--display-link]\n",
                 program);
}

// --steps N (default 480), --size WxH (starting size, default 1600x1000), --range WxH (how far
// the window shrinks, default 480x300), --amplitude WxH (points per step, default 24x15; bigger
// is a faster drag), --log (one line per step that missed), --drag (measure a real drag),
// --dark (dark palette, for judging whether colour alone changes how a resize looks),
// --display-link (in --drag, run the display link as well, which the sweep always does).
inline bool parse_options(int argc, char** argv, options* options) {
    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];
        const char* value = i + 1 < argc ? argv[i + 1] : nullptr;
        if (std::strcmp(arg, "--steps") == 0 && value) {
            options->steps = std::atoi(value);
            if (options->steps < 1) return false;
            ++i;
        } else if (std::strcmp(arg, "--size") == 0 && value) {
            if (!parse_size(value, &options->maximum)) return false;
            ++i;
        } else if (std::strcmp(arg, "--range") == 0 && value) {
            if (!parse_size(value, &options->range)) return false;
            ++i;
        } else if (std::strcmp(arg, "--amplitude") == 0 && value) {
            if (!parse_size(value, &options->amplitude)) return false;
            ++i;
        } else if (std::strcmp(arg, "--log") == 0) {
            options->log = true;
        } else if (std::strcmp(arg, "--drag") == 0) {
            options->drag = true;
        } else if (std::strcmp(arg, "--dark") == 0) {
            options->dark = true;
        } else if (std::strcmp(arg, "--display-link") == 0) {
            options->display_link = true;
        } else {
            return false;
        }
    }
    return true;
}

inline double quantile(std::vector<double> values, double q) {
    if (values.empty()) return 0.0;
    std::sort(values.begin(), values.end());
    const double index = q * static_cast<double>(values.size() - 1);
    const size_t lo = static_cast<size_t>(std::floor(index));
    const size_t hi = static_cast<size_t>(std::ceil(index));
    const double fraction = index - static_cast<double>(lo);
    return values[lo] * (1.0 - fraction) + values[hi] * fraction;
}

inline double maximum(const std::vector<double>& values) {
    return values.empty() ? 0.0 : *std::max_element(values.begin(), values.end());
}

class recorder {
public:
    recorder(const options& options, std::string backend)
        : options_(options), backend_(std::move(backend)) {}

    // Call on every display tick. In sweep mode this returns true when the app must resize the
    // window to *requested now and then call set_size_done(); the sweep ends itself after the
    // last step, so check finished() afterwards. In drag mode ticks are only the clock.
    bool tick(double now, size current, size* requested) {
        if (phase_ == Phase::done) return false;

        if (options_.drag) {
            // Only the drag itself is measured, not the idle time before it.
            if (phase_ == Phase::running && last_tick_time_ > 0.0) {
                tick_intervals_.push_back(now - last_tick_time_);
            }
            last_tick_time_ = now;
            return false;
        }

        if (phase_ == Phase::settling) {
            if (++settle_ticks_ < kSettleTicks) return false;
            phase_ = Phase::running;
            // Whatever the screen allowed is the true maximum; the sweep stays below it.
            maximum_ = current;
            half_period_ = std::max(1, static_cast<int>(std::ceil(std::max(
                                           options_.range.w / options_.amplitude.w,
                                           options_.range.h / options_.amplitude.h))));
            delta_ = size{options_.range.w / half_period_, options_.range.h / half_period_};
            start_time_ = now;
            last_tick_time_ = now;
            // The idle gap since the first paint is not a paint gap of the sweep.
            last_paint_time_ = 0.0;
            return false;
        }

        const double tick_interval = now - last_tick_time_;
        tick_intervals_.push_back(tick_interval);
        last_tick_time_ = now;

        if (step_ > 0) {
            close_step(now, tick_interval);
        }
        if (step_ == options_.steps) {
            finish(now);
            return false;
        }

        requested_ = size_for_step(step_);
        open_step(now);
        *requested = requested_;
        return true;
    }

    // Sweep mode: right after the window size was applied.
    void set_size_done(double now) { set_size_ms_.push_back((now - step_start_) * 1000.0); }

    // Drag mode: every resize event of the window. The first one is the window appearing; a
    // step only counts as continuous when it follows the previous event within 1.5 refreshes.
    void resize_event(double now, size new_size) {
        if (phase_ == Phase::done) return;
        if (phase_ == Phase::settling) {
            phase_ = Phase::running;
            start_time_ = now;
            continuous_ = false;
        } else {
            const double interval = now - step_start_;
            close_step(now, interval);
            event_intervals_.push_back(interval);
            continuous_ = interval <= kLateRefreshes * refresh_estimate();
            if (continuous_) ++continuous_events_;
        }
        end_time_ = now;
        requested_ = new_size;
        open_step(now);
    }

    // Both modes, around the app's paint. `painted` is the size the frame is drawn at.
    void paint_begin(double now, size painted) {
        in_step_ = phase_ == Phase::running && step_ > 0;
        if (in_step_) {
            ++frames_since_step_;
            if (!step_painted_) {
                step_painted_ = true;
                latency_ms_.push_back((now - step_start_) * 1000.0);
            }
            if (painted.w != requested_.w || painted.h != requested_.h) {
                ++size_mismatches_;
            }
            if (continuous_ && last_paint_time_ > 0.0) {
                step_gap_ = now - last_paint_time_;
                paint_gaps_.push_back(step_gap_);
            }
        }
        last_paint_time_ = now;
        paint_start_ = now;
    }

    void paint_end(double now) {
        if (in_step_) {
            paint_ms_.push_back((now - paint_start_) * 1000.0);
        }
    }

    // Prints the summary once. Sweep mode calls it itself after the last step.
    void finish(double now) {
        if (phase_ == Phase::done) return;
        phase_ = Phase::done;
        if (!options_.drag) end_time_ = now;
        report();
    }

    bool finished() const { return phase_ == Phase::done; }

private:
    enum class Phase { settling, running, done };

    // Triangle wave below the maximum: shrink for half a period, grow back for the other half.
    size size_for_step(int step) const {
        const int position = step % (2 * half_period_);
        const int depth = position < half_period_ ? position + 1 : 2 * half_period_ - position - 1;
        return size{std::round(maximum_.w - depth * delta_.w),
                    std::round(maximum_.h - depth * delta_.h)};
    }

    // Median tick interval when there are ticks, median event interval in a drag without a
    // display link, and the ProMotion rate until either exists.
    double refresh_estimate() const {
        if (!tick_intervals_.empty()) return quantile(tick_intervals_, 0.5);
        if (!event_intervals_.empty()) return quantile(event_intervals_, 0.5);
        return kAssumedRefresh;
    }

    void open_step(double now) {
        frames_since_step_ = 0;
        step_painted_ = false;
        step_gap_ = 0.0;
        step_start_ = now;
        ++step_;
    }

    // Called when the next step begins. `interval` is the time since the previous step began.
    void close_step(double now, double interval) {
        if (!continuous_) return;
        frames_per_step_.push_back(frames_since_step_);
        if (options_.log) {
            log_step(now, interval);
        }
    }

    void log_step(double now, double interval) const {
        const bool late_paint = step_gap_ > kLateRefreshes * refresh_estimate();
        if (frames_since_step_ == 1 && !late_paint) {
            return;
        }
        std::printf("step %4d  t=%6.3f s  %4.0fx%-4.0f  frames %d  %s %.1f ms  paint gap %.1f ms\n",
                    step_, now - start_time_, requested_.w, requested_.h, frames_since_step_,
                    options_.drag ? "event interval" : "tick", interval * 1000.0,
                    step_gap_ * 1000.0);
    }

    void report() const {
        const double refresh = refresh_estimate();
        const double elapsed = end_time_ - start_time_;

        int one = 0, none = 0, extra = 0;
        for (int frames : frames_per_step_) {
            if (frames == 1) {
                ++one;
            } else if (frames == 0) {
                ++none;
            } else {
                ++extra;
            }
        }
        const int steps = static_cast<int>(frames_per_step_.size());

        int late_ticks = 0;
        int early_ticks = 0;
        for (double interval : tick_intervals_) {
            if (interval > kLateRefreshes * refresh) ++late_ticks;
            if (interval < kEarlyRefreshes * refresh) ++early_ticks;
        }
        int late_gaps = 0;
        for (double gap : paint_gaps_) {
            if (gap > kLateRefreshes * refresh) ++late_gaps;
        }

        if (options_.drag) {
            std::printf("resize_benchmark (drag): %s, %d resize events in %.2f s, %d continuous "
                        "(within %.1f refreshes), refresh %.2f ms (%.1f Hz, from %s)\n",
                        backend_.c_str(), static_cast<int>(event_intervals_.size()) + 1, elapsed,
                        continuous_events_, kLateRefreshes, refresh * 1000.0,
                        refresh > 0.0 ? 1.0 / refresh : 0.0,
                        tick_intervals_.empty() ? "event intervals" : "display ticks");
            std::printf("event interval:   p50 %.2f  p95 %.2f  max %.1f ms\n",
                        quantile(event_intervals_, 0.5) * 1000.0,
                        quantile(event_intervals_, 0.95) * 1000.0,
                        maximum(event_intervals_) * 1000.0);
        } else {
            std::printf("resize_benchmark: %s, %.0fx%.0f down to %.0fx%.0f, %.1fx%.1f pt/step, "
                        "%d steps in %.2f s, refresh %.2f ms (%.1f Hz)\n",
                        backend_.c_str(), maximum_.w, maximum_.h, maximum_.w - options_.range.w,
                        maximum_.h - options_.range.h, delta_.w, delta_.h, steps, elapsed,
                        refresh * 1000.0, refresh > 0.0 ? 1.0 / refresh : 0.0);
        }
        std::printf("frames per step:  one %d (%.1f%%)  none %d  two or more %d\n", one,
                    steps > 0 ? 100.0 * one / steps : 0.0, none, extra);
        if (!options_.drag) {
            std::printf("set_size ms:      p50 %.2f  p95 %.2f  max %.2f\n",
                        quantile(set_size_ms_, 0.5), quantile(set_size_ms_, 0.95),
                        maximum(set_size_ms_));
        }
        std::printf("paint ms:         p50 %.2f  p95 %.2f  max %.2f\n", quantile(paint_ms_, 0.5),
                    quantile(paint_ms_, 0.95), maximum(paint_ms_));
        std::printf("resize->paint ms: p50 %.2f  p95 %.2f  max %.2f\n",
                    quantile(latency_ms_, 0.5), quantile(latency_ms_, 0.95),
                    maximum(latency_ms_));
        std::printf("paint gaps:       max %.1f ms (%.1f refreshes)  over %.1f refreshes: %d\n",
                    maximum(paint_gaps_) * 1000.0,
                    refresh > 0.0 ? maximum(paint_gaps_) / refresh : 0.0, kLateRefreshes,
                    late_gaps);
        if (!tick_intervals_.empty()) {
            const int expected_ticks =
                refresh > 0.0 ? static_cast<int>(std::round(elapsed / refresh)) : 0;
            std::printf("display ticks:    %d delivered of ~%d, late %d, early %d, max interval "
                        "%.1f ms\n",
                        static_cast<int>(tick_intervals_.size()), expected_ticks, late_ticks,
                        early_ticks, maximum(tick_intervals_) * 1000.0);
        }
        if (size_mismatches_ != 0) {
            std::printf("painted size differed from the requested size in %d frames\n",
                        size_mismatches_);
        }
    }

    options options_;
    std::string backend_;

    Phase phase_ = Phase::settling;
    int settle_ticks_ = 0;
    size maximum_;
    int half_period_ = 1;
    size delta_;
    int step_ = 0;
    size requested_;
    bool continuous_ = true;
    int continuous_events_ = 0;
    double start_time_ = 0.0;
    double end_time_ = 0.0;
    double last_tick_time_ = 0.0;
    double step_start_ = 0.0;
    bool step_painted_ = false;
    bool in_step_ = false;
    int frames_since_step_ = 0;
    double step_gap_ = 0.0;
    double last_paint_time_ = 0.0;
    double paint_start_ = 0.0;
    int size_mismatches_ = 0;

    std::vector<int> frames_per_step_;
    std::vector<double> tick_intervals_;
    std::vector<double> event_intervals_;
    std::vector<double> set_size_ms_;
    std::vector<double> paint_ms_;
    std::vector<double> latency_ms_;
    std::vector<double> paint_gaps_;
};

}  // namespace resize_sweep
