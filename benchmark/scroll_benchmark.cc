#include "px/px.h"
#include "ui/retained_text.h"
#include "ui/smooth_scroll.h"
#include <CoreFoundation/CoreFoundation.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <format>
#include <fstream>
#include <functional>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

constexpr double kWindowWidth = 1400.0;
constexpr double kWindowHeight = 800.0;
constexpr double kSidebarWidth = 260.0;
constexpr double kGutterWidth = 64.0;
constexpr double kLineHeight = 20.0;
constexpr double kTextTop = 18.0;
constexpr double kWarmupSeconds = 0.5;
constexpr double kSettleSeconds = 0.25;
constexpr double kNominalFrameInterval = 1.0 / 120.0;
// Room to scroll either way from the start: the document wraps, and a recorded trace may run in
// either direction.
constexpr double kInitialOffset = 100000.0;
constexpr double kMaximumOffset = 200000.0;
constexpr int kSettledTicks = 30;  // consecutive display ticks at one size, for --fullscreen

constexpr fcolor kWindowBackground{0.055f, 0.060f, 0.070f, 1.0f};
constexpr fcolor kSidebarBackground{0.105f, 0.115f, 0.135f, 1.0f};
constexpr fcolor kDocumentBackground{0.075f, 0.082f, 0.098f, 1.0f};
constexpr fcolor kGutterBackground{0.068f, 0.074f, 0.088f, 1.0f};

constexpr std::array<std::string_view, 20> kSourceLines = {
    "namespace editor::rendering {",
    "struct GlyphPosition { uint32_t id; float x; float advance; };",
    "constexpr double kLineHeight = 20.0; // logical points",
    "const auto visible = view.visible_region().expanded_by(2);",
    "for (const DisplayLine& line : visible.lines()) {",
    "    const ShapedLine& shaped = cache.shape(line.text());",
    "    renderer.draw_text(shaped, origin + line.offset());",
    "    atlas.upload_missing_glyphs(shaped.glyphs());",
    "}",
    "context.restrict_clip_rect(document_bounds);",
    "selection.paint(context, layout, theme.selection_color());",
    "scrollbar.set_position(view.scroll_offset() / document.height());",
    "Ligatures: fi ffi fl <= != -> => === !==",
    "Fallback: 你好世界  Καλημέρα  привет  مرحبا  שלום",
    "Emoji and color glyphs: 👋 🌍 ✨ 🚀",
    "auto frame = compositor.acquire_frame(viewport.device_size());",
    "frame.clear(Color{0.075f, 0.082f, 0.098f, 1.0f});",
    "glyph_batch.flush(atlas.texture(), BlendMode::source_over);",
    "presenter.submit(std::move(frame));",
    "}  // namespace editor::rendering",
};

constexpr std::array<std::string_view, 10> kSidebarLines = {
    "FOLDERS", "simple-text", "experiments", "platform",    "benchmark", "scroll_benchmark.cc",
    "px",      "editor.cc",   "README.md",   "third_party",
};

using PreparedText = retained_text;

PreparedText prepare_text(px_font_t* font, std::string_view text) {
    grapheme_shaper* shaper = grapheme_shaper::instance(font);
    return prepare_retained_text(shaper, text);
}

struct PreparedLine {
    PreparedText text;
    fcolor color;
};

// ── the trace ───────────────────────────────────────────────────────────────────────────────────
//
// A recorded scroll: every scroll event the window received, timed from the first one, with the
// delta as px delivered it. Written by --record and played back into the same scene by --replay.

constexpr char kTraceHeader[] = "# px-scroll-trace-v2";

struct Sample {
    uint64_t time_ns = 0;
    double delta_x = 0.0;
    double delta_y = 0.0;
    bool precise = false;  // trackpad points; false is a wheel's lines
};

bool parse_sample(const std::string& line, Sample* sample) {
    int precise = 0;
    std::istringstream input(line);
    if (!(input >> sample->time_ns >> sample->delta_x >> sample->delta_y >> precise)) {
        return false;
    }
    input >> std::ws;
    sample->precise = precise != 0;
    return input.eof() && std::isfinite(sample->delta_x) && std::isfinite(sample->delta_y);
}

bool read_trace(const char* path, std::vector<Sample>* samples, std::string* error) {
    std::ifstream input(path);
    if (!input) {
        *error = std::format("cannot open {}", path);
        return false;
    }
    std::string line;
    bool found_header = false;
    size_t line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        if (line == kTraceHeader) {
            found_header = true;
            continue;
        }
        if (line.empty() || line[0] == '#') {
            continue;
        }
        Sample sample;
        if (!parse_sample(line, &sample)) {
            *error = std::format("malformed sample at {}:{}", path, line_number);
            return false;
        }
        if (!samples->empty() && sample.time_ns < samples->back().time_ns) {
            *error = std::format("timestamps go backwards at {}:{}", path, line_number);
            return false;
        }
        samples->push_back(sample);
    }
    if (!found_header) {
        *error = std::format("{} is not a version-2 scroll trace", path);
        return false;
    }
    if (samples->empty()) {
        *error = std::format("{} contains no samples", path);
        return false;
    }
    return true;
}

bool write_trace(const char* path, const std::vector<Sample>& samples, std::string* error) {
    std::ofstream output(path);
    if (!output) {
        *error = std::format("cannot create {}", path);
        return false;
    }
    output
        << kTraceHeader << '\n'
        << "# time_ns\tdelta_x\tdelta_y\tprecise\n"
           "# deltas as px delivered them; precise is 1 for trackpad points, 0 for wheel lines\n";
    for (const Sample& sample : samples) {
        output << std::format("{}\t{}\t{}\t{}\n", sample.time_ns, sample.delta_x, sample.delta_y,
                              sample.precise ? 1 : 0);
    }
    if (!output) {
        *error = std::format("cannot write {}", path);
        return false;
    }
    return true;
}

// ── timed delivery ──────────────────────────────────────────────────────────────────────────────
//
// Delivers the trace's timestamps on the main thread, independently of the display clock: a
// thread sleeps until each sample is due and posts it over with a zero-delay timeout, which is
// dispatch_after on macOS and safe from any thread. One timer at a time is the point. Queuing a
// timeout per sample up front hands each one leeway proportional to how far ahead it is set,
// hundreds of milliseconds by the end of a trace, which bunches 120 Hz samples into bursts.
//
// The callback receives the time the sample was scheduled for, on the px_now() clock. That plays
// the part of a native event's own timestamp: the time the main thread gets to it is jittered by
// whatever the thread was doing, as with a real event.
void schedule_timed_input(const std::vector<uint64_t>& timestamps_ns,
                          uint64_t phase_ns,
                          std::function<void(size_t index, double scheduled_time)> callback) {
    std::thread([timestamps_ns, phase_ns, callback = std::move(callback)] {
        auto start = std::chrono::steady_clock::now() + std::chrono::nanoseconds(phase_ns);
        double start_seconds = px_now() + static_cast<double>(phase_ns) / 1e9;
        for (size_t index = 0; index < timestamps_ns.size(); ++index) {
            std::this_thread::sleep_until(start + std::chrono::nanoseconds(timestamps_ns[index]));
            double scheduled = start_seconds + static_cast<double>(timestamps_ns[index]) / 1e9;
            px_set_timeout([callback, index, scheduled] { callback(index, scheduled); }, 0);
        }
    }).detach();
}

struct Options {
    const char* record_path = nullptr;
    const char* replay_path = nullptr;
    int repetitions = 1;
    double input_phase_ms = 4.0;
    bool dump_frames = false;
    bool keep_open = false;
    bool full_screen = false;
    // --record: end the recording this long after the last scroll event (0: Escape ends it).
    double stop_after_idle_s = 0.0;
    // The editor's path for this build by default (PX_OS_SMOOTHS_EVENT_FRAMES in px/px.h):
    // per-event, every event adds its delta and commits its own frame with nothing sampled from
    // the tick (Sublime's model, the editor's path on a build linked against the macOS 26 SDK);
    // or tick, the display link sampling the input's trajectory (ui/smooth_scroll). --per-event
    // and --tick choose the other for comparison.
    bool per_event = PX_OS_SMOOTHS_EVENT_FRAMES;
};

bool parse_positive_int(const char* text, int maximum, int* value) {
    if (!text || !*text) {
        return false;
    }
    char* end = nullptr;
    const long parsed = std::strtol(text, &end, 10);
    if (!end || *end != '\0' || parsed < 1 || parsed > maximum) {
        return false;
    }
    *value = static_cast<int>(parsed);
    return true;
}

void usage(const char* program) {
    std::fprintf(
        stderr,
        "usage: %s --record OUT.tsv [--stop-after-idle S] | --replay TRACE.tsv "
        "[--input-phase-ms N] [--repetitions N] [--dump-frames] [--keep-open] [--fullscreen] "
        "[--per-event | --tick]\n",
        program);
}

bool parse_options(int argc, char** argv, Options* options) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            std::exit(0);
        } else if (std::strcmp(argv[i], "--record") == 0 && i + 1 < argc) {
            options->record_path = argv[++i];
        } else if (std::strcmp(argv[i], "--replay") == 0 && i + 1 < argc) {
            options->replay_path = argv[++i];
        } else if (std::strcmp(argv[i], "--input-phase-ms") == 0 && i + 1 < argc) {
            char* end = nullptr;
            options->input_phase_ms = std::strtod(argv[++i], &end);
            if (!end || *end != '\0' || !std::isfinite(options->input_phase_ms) ||
                options->input_phase_ms < 0.0 || options->input_phase_ms > 100.0) {
                return false;
            }
        } else if (std::strcmp(argv[i], "--repetitions") == 0 && i + 1 < argc) {
            if (!parse_positive_int(argv[++i], 100, &options->repetitions)) {
                return false;
            }
        } else if (std::strcmp(argv[i], "--dump-frames") == 0) {
            options->dump_frames = true;
        } else if (std::strcmp(argv[i], "--keep-open") == 0) {
            options->keep_open = true;
        } else if (std::strcmp(argv[i], "--fullscreen") == 0) {
            options->full_screen = true;
        } else if (std::strcmp(argv[i], "--stop-after-idle") == 0 && i + 1 < argc) {
            char* end = nullptr;
            options->stop_after_idle_s = std::strtod(argv[++i], &end);
            if (!end || *end != '\0' || !std::isfinite(options->stop_after_idle_s) ||
                options->stop_after_idle_s <= 0.0) {
                return false;
            }
        } else if (std::strcmp(argv[i], "--per-event") == 0) {
            options->per_event = true;
        } else if (std::strcmp(argv[i], "--tick") == 0) {
            options->per_event = false;
        } else {
            return false;
        }
    }
    return (options->record_path != nullptr) != (options->replay_path != nullptr);
}

double quantile(std::vector<double> values, double q) {
    std::sort(values.begin(), values.end());
    const double index = q * static_cast<double>(values.size() - 1);
    const size_t low = static_cast<size_t>(std::floor(index));
    const size_t high = static_cast<size_t>(std::ceil(index));
    const double fraction = index - static_cast<double>(low);
    return values[low] * (1.0 - fraction) + values[high] * fraction;
}

void print_distribution(const char* name, const char* unit, const std::vector<double>& values) {
    if (values.empty()) {
        std::printf("%s n=0\n", name);
        return;
    }
    std::printf("%s n=%zu p50=%.3f%s p95=%.3f%s p99=%.3f%s max=%.3f%s\n", name, values.size(),
                quantile(values, 0.50), unit, quantile(values, 0.95), unit, quantile(values, 0.99),
                unit, *std::max_element(values.begin(), values.end()), unit);
}

size_t wrapped_index(int64_t index, size_t count) {
    const int64_t signed_count = static_cast<int64_t>(count);
    const int64_t remainder = index % signed_count;
    return static_cast<size_t>(remainder < 0 ? remainder + signed_count : remainder);
}

class ScrollBenchmark final : public px_window_event_handler, public px_application_event_handler {
public:
    ScrollBenchmark(Options options, std::vector<Sample> samples)
        : options_(options), samples_(std::move(samples)) {
        body_font_ = px_create_font("Source Code Pro", 15.0f);
        ui_font_ = px_create_font("system", 12.0f);
        heading_font_ = px_create_font("system", 12.0f, PX_FONT_BOLD);

        lines_.reserve(kSourceLines.size());
        for (size_t i = 0; i < kSourceLines.size(); ++i) {
            const std::array<fcolor, 5> colors = {
                fcolor{0.78f, 0.80f, 0.86f, 1.0f}, fcolor{0.48f, 0.72f, 0.96f, 1.0f},
                fcolor{0.72f, 0.52f, 0.91f, 1.0f}, fcolor{0.91f, 0.58f, 0.36f, 1.0f},
                fcolor{0.50f, 0.75f, 0.58f, 1.0f},
            };
            lines_.push_back(PreparedLine{prepare_text(body_font_, kSourceLines[i]),
                                          colors[i % colors.size()]});
        }
        for (std::string_view text : kSidebarLines) {
            sidebar_.push_back(prepare_text(ui_font_, text));
        }
        line_numbers_.reserve(256);
        for (int i = 1; i <= 256; ++i) {
            line_numbers_.push_back(prepare_text(ui_font_, std::to_string(i)));
        }

        for (const Sample& sample : samples_) {
            input_distance_ += std::abs(sample.delta_y);
        }
    }

    void attach(px_window_t* window) {
        window_ = window;
        // The main thread's run loop, watched passively: each wake-to-sleep segment is the time
        // the thread was busy (event handling, display, Core Animation's commit, which runs as
        // an observer ahead of this one), each sleep-to-wake segment the time it waited.
        CFRunLoopObserverContext context{0, this, nullptr, nullptr, nullptr};
        CFRunLoopObserverRef observer = CFRunLoopObserverCreate(
            kCFAllocatorDefault, kCFRunLoopAfterWaiting | kCFRunLoopBeforeWaiting, true, 3000000,
            &run_loop_activity, &context);
        CFRunLoopAddObserver(CFRunLoopGetMain(), observer, kCFRunLoopCommonModes);
    }

    static void run_loop_activity(CFRunLoopObserverRef, CFRunLoopActivity activity, void* info) {
        ScrollBenchmark* self = static_cast<ScrollBenchmark*>(info);
        const double now = px_now();
        const bool tracking = self->playback_start_time_ != 0.0 && !self->reported_;
        if (activity == kCFRunLoopAfterWaiting) {
            if (tracking && self->slept_at_ != 0.0) {
                self->run_loop_sleep_ms_.push_back((now - self->slept_at_) * 1000.0);
            }
            self->woke_at_ = now;
        } else {
            if (tracking && self->woke_at_ != 0.0) {
                self->run_loop_busy_ms_.push_back((now - self->woke_at_) * 1000.0);
            }
            self->slept_at_ = now;
        }
    }

    // --record: reports what the live gesture did on the glass and writes what was scrolled,
    // once, on Escape, Cmd+Q or the idle stop.
    void write_recording() {
        if (!options_.record_path || recording_written_) {
            return;
        }
        recording_written_ = true;
        if (recording_.empty()) {
            std::printf("nothing recorded: no scroll event arrived\n");
            return;
        }
        if (!reported_) {
            report();
            reported_ = true;
            px_set_frame_presented_callback(window_, {});
        }
        std::string error;
        if (!write_trace(options_.record_path, recording_, &error)) {
            std::fprintf(stderr, "scroll_benchmark: %s\n", error.c_str());
            return;
        }
        std::printf("recorded samples=%zu duration=%.3fs to %s\n", recording_.size(),
                    static_cast<double>(recording_.back().time_ns) / 1e9, options_.record_path);
    }

    bool can_quit_without_prompt() override {
        write_recording();
        return true;
    }

    bool handle_event(px_event_t* event) override {
        if (event->type == PX_EVENT_KEY && event->pressed && event->key == PX_KEY_ESCAPE) {
            write_recording();
            px_close_window(window_);
            return true;
        }
        // Hand scrolling: what --record captures, and what --keep-open leaves to feel afterwards.
        // Refused while a trace is playing, since the offset belongs to the trace then, and
        // letting the trackpad move it too would quietly corrupt the numbers this harness exists
        // to produce.
        if (event->type == PX_EVENT_SCROLL && (options_.record_path || reported_)) {
            record(event);
            if (options_.record_path) {
                note_live_input(event);
            }
            apply_scroll(-event->scroll_delta.y, event->timestamp, event->precise_scroll);
            return true;
        }
        return false;
    }

    void animation_tick(double now) override {
        if (options_.record_path) {
            // Recording: the live path, as the editor drives it. The offset starts mid-document
            // so the recording can scroll either way, as a replay of it will. The ticks are only
            // telemetry here, as on the replay's per-event path.
            if (first_tick_time_ == 0.0) {
                first_tick_time_ = now;
                scroll_.jump_to(kInitialOffset, kMaximumOffset);
                px_mark_dirty(window_);
            }
            tick_target_time_ = now;
            tick_delay_ms_ = (px_now() - (now - kNominalFrameInterval)) * 1000.0;
            if (playback_start_time_ != 0.0 && !reported_) {
                if (previous_tick_time_ != 0.0) {
                    tick_intervals_ms_.push_back((now - previous_tick_time_) * 1000.0);
                }
                previous_tick_time_ = now;
                if (options_.stop_after_idle_s > 0.0 &&
                    px_now() - last_live_event_time_ >= options_.stop_after_idle_s) {
                    write_recording();
                    px_set_timeout([window = window_] { px_close_window(window); }, 0);
                    return;
                }
            }
            if (scroll_.tick(px_now(), kMaximumOffset)) {
                px_mark_dirty(window_);
            }
            return;
        }
        tick_target_time_ = now;
        tick_delay_ms_ = (px_now() - (now - kNominalFrameInterval)) * 1000.0;
        if (first_tick_time_ == 0.0) {
            first_tick_time_ = now;
            px_mark_dirty(window_);
            return;
        }
        if (playback_start_time_ == 0.0) {
            if (!warmed_up(now)) {
                px_mark_dirty(window_);
                return;
            }
            playback_start_time_ = px_now();
            previous_tick_time_ = now;
            scroll_.jump_to(kInitialOffset, kMaximumOffset);
            previous_rendered_offset_ = scroll_.offset();
            previous_presented_offset_ = scroll_.offset();
            last_painted_offset_ = scroll_.offset();
            px_set_frame_presented_callback(window_,
                                            [this](uint64_t frame_id, double presented_time) {
                                                frame_presented(frame_id, presented_time);
                                            });
            schedule_input();
            std::printf("playback_started input_phase=%.3fms target_time=%.6f\n",
                        options_.input_phase_ms, now);
        } else if (delivered_samples_ < samples_.size()) {
            tick_intervals_ms_.push_back((now - previous_tick_time_) * 1000.0);
            previous_tick_time_ = now;
        }

        if (scroll_.tick(px_now(), kMaximumOffset)) {
            px_mark_dirty(window_);
        }

        const double elapsed = std::max(0.0, px_now() - playback_start_time_);
        const double trace_end = static_cast<double>(samples_.back().time_ns) / 1'000'000'000.0;
        const double phase = options_.input_phase_ms / 1000.0;
        if (!reported_ && delivered_samples_ == samples_.size() &&
            elapsed >= trace_end + phase + kSettleSeconds) {
            report();
            reported_ = true;
            px_set_frame_presented_callback(window_, {});
            if (options_.keep_open) {
                std::printf("playback_finished scroll_to_explore=1 quit=escape\n");
            } else {
                px_set_timeout([window = window_] { px_close_window(window); }, 0);
            }
        }
    }

    void paint(px_render_context* context,
               rect bounds,
               const rect* dirty,
               int dirty_count) override {
        if (!backend_) {
            backend_ = context->supports_batching() ? "gpu-batched" : "software";
        }

        const auto begin = std::chrono::steady_clock::now();
        draw_scene(context, bounds);
        const auto end = std::chrono::steady_clock::now();

        const double scroll_offset = scroll_.offset();
        if (playback_start_time_ == 0.0 || reported_) {
            return;
        }
        const double paint_time = px_now();
        if (scroll_offset == last_painted_offset_) {
            // A paint something other than the scroll asked for (the unchanged frame committed
            // when a gesture starts). It still occupies a refresh on the glass, and can be the
            // frame the window server keeps when two land in one refresh, so it is tracked for
            // presentation; it is no paint of the motion, so it counts toward nothing else.
            const uint64_t frame_id = px_current_frame_id(window_);
            if (frame_id != 0) {
                paint_times_[frame_id] = paint_time;
                awaiting_presentation_.insert_or_assign(
                    frame_id, painted_frame{.paint_time = paint_time,
                                            .tick_target_time = tick_target_time_,
                                            .tick_delay_ms = tick_delay_ms_,
                                            .last_input_time = last_input_time_,
                                            .offset = scroll_offset,
                                            .unchanged = true});
            }
            return;
        }
        if (previous_paint_time_ != 0.0) {
            paint_intervals_ms_.push_back((paint_time - previous_paint_time_) * 1000.0);
        }
        previous_paint_time_ = paint_time;
        if (last_input_time_ != 0.0) {
            input_to_paint_ms_.push_back((paint_time - last_input_time_) * 1000.0);
        }
        render_times_ms_.push_back(std::chrono::duration<double, std::milli>(end - begin).count());
        events_per_paint_.push_back(static_cast<double>(pending_events_));
        const uint64_t frame_id = px_current_frame_id(window_);
        if (frame_id != 0) {
            paint_times_[frame_id] = paint_time;
            awaiting_presentation_.insert_or_assign(
                frame_id, painted_frame{.paint_time = paint_time,
                                        .tick_target_time = tick_target_time_,
                                        .tick_delay_ms = tick_delay_ms_,
                                        .last_input_time = last_input_time_,
                                        .offset = scroll_offset});
        }
        last_painted_offset_ = scroll_offset;

        const double motion_step = std::abs(scroll_offset - previous_rendered_offset_);
        if (motion_step != 0.0) {
            motion_steps_.push_back(motion_step);
            displayed_distance_ += motion_step;
            previous_rendered_offset_ = scroll_offset;
        }
        if (pending_motion_ticks_ > 1) {
            coalesced_motion_ticks_ += pending_motion_ticks_ - 1;
        }
        if (options_.dump_frames) {
            std::printf("FRAME time=%.6f offset=%.3f events=%zu motion_ticks=%zu render=%.3fms\n",
                        paint_time - playback_start_time_, scroll_offset, pending_events_,
                        pending_motion_ticks_, render_times_ms_.back());
        }
        pending_events_ = 0;
        pending_motion_ticks_ = 0;
        ++paint_count_;
    }

private:
    struct painted_frame {
        double paint_time = 0.0;
        double tick_target_time = 0.0;
        double tick_delay_ms = 0.0;
        double last_input_time = 0.0;
        double offset = 0.0;
        bool unchanged = false;
    };

    void schedule_input() {
        std::vector<uint64_t> timestamps;
        timestamps.reserve(samples_.size());
        for (const Sample& sample : samples_) {
            timestamps.push_back(sample.time_ns);
        }
        schedule_timed_input(timestamps,
                             static_cast<uint64_t>(options_.input_phase_ms * 1'000'000.0),
                             [this](size_t index, double scheduled_time) {
                                 deliver_sample(index, scheduled_time);
                             });
    }

    // `now` is the sample's scheduled time, standing in for a native event timestamp.
    void deliver_sample(size_t index, double now) {
        if (reported_ || index >= samples_.size()) {
            return;
        }

        sample_delivery_lag_ms_.push_back((px_now() - now) * 1000.0);
        const Sample& sample = samples_[index];
        const double delta = -sample.delta_y;
        apply_scroll(delta, now, sample.precise);
        last_input_time_ = now;
        ++delivered_samples_;
        ++input_ticks_;
        ++pending_events_;
        if (delta != 0.0) {
            ++motion_ticks_;
            ++pending_motion_ticks_;
        }
    }

    // The editor's route for a scroll event (experiments/examples/editor.cc). Per-event, every
    // event with motion commits its own frame; on the tick path a precise event feeds the
    // trajectory and the display link paints, and a wheel's lines jump either way.
    void apply_scroll(double delta, double timestamp, bool precise) {
        if (options_.per_event || !precise) {
            if (delta != 0.0) {
                scroll_.jump_to(scroll_.offset() + delta, kMaximumOffset);
                px_mark_dirty(window_);
            }
        } else if (scroll_.scroll(delta, timestamp, kMaximumOffset)) {
            px_mark_dirty(window_);
        }
    }

    const char* path_name() const { return options_.per_event ? "per-event" : "tick"; }

    // --record: the live gesture is measured as a replay is, from the event's own timestamp,
    // with the recording itself as the ideal trajectory.
    void note_live_input(const px_event_t* event) {
        if (playback_start_time_ == 0.0) {
            playback_start_time_ = recording_start_time_;
            previous_rendered_offset_ = scroll_.offset();
            previous_presented_offset_ = scroll_.offset();
            last_painted_offset_ = scroll_.offset();
            px_set_frame_presented_callback(window_,
                                            [this](uint64_t frame_id, double presented_time) {
                                                frame_presented(frame_id, presented_time);
                                            });
        }
        sample_delivery_lag_ms_.push_back((px_now() - event->timestamp) * 1000.0);
        last_input_time_ = event->timestamp;
        last_live_event_time_ = px_now();
        ++input_ticks_;
        ++pending_events_;
        if (event->scroll_delta.y != 0.0) {
            ++motion_ticks_;
            ++pending_motion_ticks_;
        }
    }

    void record(const px_event_t* event) {
        if (!options_.record_path) {
            return;
        }
        if (recording_.empty()) {
            recording_start_time_ = event->timestamp;
        }
        input_distance_ += std::abs(event->scroll_delta.y);
        uint64_t time_ns = static_cast<uint64_t>(
            std::llround(std::max(0.0, event->timestamp - recording_start_time_) * 1e9));
        if (!recording_.empty()) {
            time_ns = std::max(time_ns, recording_.back().time_ns);
        }
        recording_.push_back(Sample{.time_ns = time_ns,
                                    .delta_x = event->scroll_delta.x,
                                    .delta_y = event->scroll_delta.y,
                                    .precise = event->precise_scroll});
    }

    double ideal_offset_at(double elapsed) const {
        elapsed -= options_.input_phase_ms / 1000.0;
        if (elapsed < 0.0) {
            return kInitialOffset;
        }

        const uint64_t elapsed_ns = static_cast<uint64_t>(elapsed * 1'000'000'000.0);
        double before = kInitialOffset;
        uint64_t before_time = 0;
        for (const Sample& sample : options_.record_path ? recording_ : samples_) {
            const double after = before - sample.delta_y;
            if (sample.time_ns > elapsed_ns) {
                if (sample.time_ns == before_time) {
                    return after;
                }
                const double alpha = static_cast<double>(elapsed_ns - before_time) /
                                     static_cast<double>(sample.time_ns - before_time);
                return before + (after - before) * alpha;
            }
            before = after;
            before_time = sample.time_ns;
        }
        return before;
    }

    void frame_presented(uint64_t frame_id, double presented_time) {
        const auto found = awaiting_presentation_.find(frame_id);
        if (found == awaiting_presentation_.end()) {
            return;
        }
        const painted_frame frame = found->second;
        awaiting_presentation_.erase(found);
        if (presented_time <= 0.0) {
            // A frame another paint replaced within the refresh (the gesture-start commit, or
            // the second event of a 240 Hz pair) was never going to show; the server can only
            // show one frame per refresh. Only the rest are the server's discards.
            const auto next = paint_times_.find(frame_id + 1);
            if (next != paint_times_.end() &&
                next->second - frame.paint_time < kNominalFrameInterval) {
                ++superseded_presentations_;
            } else {
                ++dropped_presentations_;
            }
            return;
        }

        if (previous_presentation_time_ != 0.0) {
            presentation_intervals_ms_.push_back((presented_time - previous_presentation_time_) *
                                                 1000.0);
        }
        previous_presentation_time_ = presented_time;
        if (frame.unchanged) {
            ++unchanged_presentations_;
            if (options_.dump_frames) {
                std::printf("PRESENT time=%.6f offset=%.3f unchanged=1\n",
                            presented_time - playback_start_time_, frame.offset);
            }
            return;
        }
        paint_to_present_ms_.push_back((presented_time - frame.paint_time) * 1000.0);
        target_to_present_ms_.push_back((presented_time - frame.tick_target_time) * 1000.0);
        tick_delays_ms_.push_back(frame.tick_delay_ms);
        if (frame.last_input_time != 0.0 && presented_time >= frame.last_input_time) {
            input_to_present_ms_.push_back((presented_time - frame.last_input_time) * 1000.0);
        }

        const double ideal = ideal_offset_at(presented_time - playback_start_time_);
        position_errors_.push_back(std::abs(frame.offset - ideal));
        if (have_previous_presented_frame_) {
            const double actual_step = frame.offset - previous_presented_offset_;
            const double ideal_step = ideal - previous_presented_ideal_;
            presented_step_errors_.push_back(std::abs(actual_step - ideal_step));
            if (std::abs(ideal_step) >= 0.25 && std::abs(actual_step) < 0.01) {
                ++stalled_presentations_;
            }
        }
        previous_presented_offset_ = frame.offset;
        previous_presented_ideal_ = ideal;
        have_previous_presented_frame_ = true;

        if (options_.dump_frames) {
            std::printf("PRESENT time=%.6f offset=%.3f ideal=%.3f error=%.3f "
                        "target_to_present=%.3fms tick_delay=%.3fms\n",
                        presented_time - playback_start_time_, frame.offset, ideal,
                        std::abs(frame.offset - ideal),
                        (presented_time - frame.tick_target_time) * 1000.0, frame.tick_delay_ms);
        }
    }

    // Entering full screen animates the window size over several frames. Sampling before it
    // settles would measure the resize rather than the scroll, so wait for the size to hold steady
    // instead of guessing at a fixed delay.
    bool warmed_up(double now) {
        if (now - first_tick_time_ < kWarmupSeconds) {
            return false;
        }
        if (!options_.full_screen) {
            return true;
        }
        const vec2 size = px_window_size(window_);
        if (size.x != settled_size_.x || size.y != settled_size_.y) {
            settled_size_ = size;
            settled_ticks_ = 0;
            return false;
        }
        return ++settled_ticks_ >= kSettledTicks;
    }

    static void draw_batches(px_render_context* context,
                             px_font_t* font,
                             vec2 origin,
                             fcolor color,
                             PreparedText* text) {
        draw_retained_text(context, font, origin, color, text);
    }

    void draw_scene(px_render_context* context, rect viewport) {
        context->begin_rect_batch();
        context->draw_rect(viewport, kWindowBackground);
        context->draw_rect(rect{0.0, 0.0, kSidebarWidth, viewport.h}, kSidebarBackground);
        context->draw_rect(rect{0.0, 36.0, kSidebarWidth, 24.0},
                           fcolor{0.18f, 0.35f, 0.58f, 1.0f});
        context->draw_rect(rect{kSidebarWidth - 1.0, 0.0, 1.0, viewport.h},
                           fcolor{0.20f, 0.22f, 0.26f, 1.0f});
        context->end_rect_batch();

        context->begin_text_batch();
        draw_batches(context, heading_font_, vec2{16.0, 25.0}, fcolor{0.72f, 0.75f, 0.82f, 1.0f},
                     &sidebar_[0]);
        for (size_t i = 1; i < sidebar_.size(); ++i) {
            draw_batches(context, ui_font_,
                         vec2{18.0 + static_cast<double>(i % 4) * 11.0,
                              25.0 + static_cast<double>(i) * 25.0},
                         fcolor{0.72f, 0.75f, 0.82f, 1.0f}, &sidebar_[i]);
        }
        context->end_text_batch();

        draw_document(context, viewport);
    }

    void draw_document(px_render_context* context, rect viewport) {
        const double document_left = kSidebarWidth + kGutterWidth;
        const rect document_clip{kSidebarWidth, 0.0, viewport.w - kSidebarWidth, viewport.h};
        const int visible_rows = static_cast<int>(std::ceil(viewport.h / kLineHeight)) + 2;
        const double scroll_offset = scroll_.offset();
        const int64_t first_line = static_cast<int64_t>(std::floor(scroll_offset / kLineHeight));
        const double fractional_scroll = scroll_offset - first_line * kLineHeight;

        context->push_state(false);
        context->restrict_clip_rect(document_clip);
        context->begin_rect_batch();
        context->draw_rect(rect{kSidebarWidth, 0.0, kGutterWidth, viewport.h}, kGutterBackground);
        context->draw_rect(rect{document_left, 0.0, viewport.w - document_left, viewport.h},
                           kDocumentBackground);
        context->draw_rect(rect{document_left - 1.0, 0.0, 1.0, viewport.h},
                           fcolor{0.16f, 0.17f, 0.20f, 1.0f});
        for (int repetition = 0; repetition < options_.repetitions; ++repetition) {
            const double offset = static_cast<double>(repetition % 4) * 0.25;
            for (int row = -1; row < visible_rows; ++row) {
                const int64_t line = first_line + row;
                const double y = kTextTop + row * kLineHeight - fractional_scroll + offset;
                if ((line + repetition) % 11 == 0) {
                    context->draw_rect(
                        rect{document_left, y - 14.0, viewport.w - document_left, kLineHeight},
                        fcolor{0.105f, 0.135f, 0.185f, 0.32f});
                }
            }
        }
        const double thumb_progress = std::fmod(std::abs(scroll_offset), 4000.0) / 4000.0;
        context->draw_rect(rect{viewport.w - 8.0, thumb_progress * (viewport.h - 80.0), 5.0, 80.0},
                           fcolor{0.38f, 0.42f, 0.50f, 0.9f});
        context->end_rect_batch();

        context->begin_text_batch();
        for (int repetition = 0; repetition < options_.repetitions; ++repetition) {
            const double offset = static_cast<double>(repetition % 4) * 0.25;
            const float alpha = repetition == 0 ? 1.0f : 0.22f;
            for (int row = -1; row < visible_rows; ++row) {
                const int64_t line = first_line + row;
                const double y = kTextTop + row * kLineHeight - fractional_scroll + offset;
                const size_t line_index = wrapped_index(line, lines_.size());
                const size_t number_index = wrapped_index(line, line_numbers_.size());
                fcolor color = lines_[line_index].color;
                color.a = alpha;
                draw_batches(context, body_font_, vec2{document_left + 10.0, y}, color,
                             &lines_[line_index].text);
                draw_batches(context, ui_font_, vec2{kSidebarWidth + 12.0, y},
                             fcolor{0.48f, 0.50f, 0.56f, alpha}, &line_numbers_[number_index]);
            }
        }
        context->end_text_batch();
        context->pop_state();
    }

    void report() {
        const std::vector<Sample>& input = options_.record_path ? recording_ : samples_;
        const double duration_ms = static_cast<double>(input.back().time_ns) / 1'000'000.0;
        const double refresh_ms =
            tick_intervals_ms_.empty() ? 0.0 : quantile(tick_intervals_ms_, 0.50);
        size_t missed_display_ticks = 0;
        if (refresh_ms > 0.0) {
            for (double interval : tick_intervals_ms_) {
                if (interval > refresh_ms * 1.5) {
                    missed_display_ticks += static_cast<size_t>(
                        std::max(0.0, std::round(interval / refresh_ms) - 1.0));
                }
            }
        }

        const vec2 size = px_window_size(window_);
        std::printf("benchmark=scroll mode=%s path=%s backend=%s trace=%s samples=%zu "
                    "trace_duration=%.3fms input_phase=%.3fms repetitions=%d "
                    "input_distance=%.3fpt presentation=%s viewport=%.0fx%.0f\n",
                    options_.record_path ? "record" : "replay", path_name(),
                    backend_ ? backend_ : "unknown",
                    options_.record_path ? options_.record_path : options_.replay_path,
                    input.size(), duration_ms, options_.input_phase_ms, options_.repetitions,
                    input_distance_, options_.full_screen ? "fullscreen" : "windowed", size.x,
                    size.y);
        print_distribution("sample_delivery_lag", "ms", sample_delivery_lag_ms_);
        print_distribution("display_tick_interval", "ms", tick_intervals_ms_);
        print_distribution("paint_interval", "ms", paint_intervals_ms_);
        print_distribution("input_to_paint", "ms", input_to_paint_ms_);
        print_distribution("run_loop_busy", "ms", run_loop_busy_ms_);
        print_distribution("run_loop_sleep", "ms", run_loop_sleep_ms_);
        print_distribution("presentation_interval", "ms", presentation_intervals_ms_);
        print_distribution("render_submit", "ms", render_times_ms_);
        print_distribution("paint_to_present", "ms", paint_to_present_ms_);
        print_distribution("target_to_present", "ms", target_to_present_ms_);
        print_distribution("tick_delay", "ms", tick_delays_ms_);
        print_distribution("input_to_present", "ms", input_to_present_ms_);
        print_distribution("presented_position_error", "pt", position_errors_);
        print_distribution("presented_step_error", "pt", presented_step_errors_);
        print_distribution("motion_step", "pt", motion_steps_);
        print_distribution("events_per_paint", "", events_per_paint_);
        std::printf("summary input_ticks=%zu motion_ticks=%zu paints=%zu missed_display_ticks=%zu "
                    "coalesced_motion_ticks=%zu pending_events=%zu displayed_distance=%.3fpt "
                    "distance_ratio=%.6f presented_frames=%zu stalled_presentations=%zu "
                    "dropped_presentations=%zu superseded_presentations=%zu "
                    "pending_presentations=%zu unchanged_presentations=%zu\n",
                    input_ticks_, motion_ticks_, paint_count_, missed_display_ticks,
                    coalesced_motion_ticks_, pending_events_, displayed_distance_,
                    input_distance_ == 0.0 ? 1.0 : displayed_distance_ / input_distance_,
                    position_errors_.size(), stalled_presentations_, dropped_presentations_,
                    superseded_presentations_, awaiting_presentation_.size(),
                    unchanged_presentations_);
    }

    Options options_;
    std::vector<Sample> samples_;
    std::vector<Sample> recording_;
    double recording_start_time_ = 0.0;
    bool recording_written_ = false;
    px_window_t* window_ = nullptr;
    px_font_t* body_font_ = nullptr;
    px_font_t* ui_font_ = nullptr;
    px_font_t* heading_font_ = nullptr;
    std::vector<PreparedLine> lines_;
    std::vector<PreparedText> sidebar_;
    std::vector<PreparedText> line_numbers_;
    const char* backend_ = nullptr;
    vec2 settled_size_;
    int settled_ticks_ = 0;
    size_t delivered_samples_ = 0;
    size_t input_ticks_ = 0;
    size_t motion_ticks_ = 0;
    size_t pending_events_ = 0;
    size_t pending_motion_ticks_ = 0;
    size_t coalesced_motion_ticks_ = 0;
    size_t paint_count_ = 0;
    double first_tick_time_ = 0.0;
    double playback_start_time_ = 0.0;
    double previous_tick_time_ = 0.0;
    double previous_paint_time_ = 0.0;
    double previous_presentation_time_ = 0.0;
    double tick_target_time_ = 0.0;
    double tick_delay_ms_ = 0.0;
    double last_input_time_ = 0.0;
    double last_live_event_time_ = 0.0;
    double last_painted_offset_ = 0.0;
    double previous_rendered_offset_ = 0.0;
    double previous_presented_offset_ = 0.0;
    double previous_presented_ideal_ = 0.0;
    double input_distance_ = 0.0;
    double displayed_distance_ = 0.0;
    bool reported_ = false;
    bool have_previous_presented_frame_ = false;
    size_t stalled_presentations_ = 0;
    size_t dropped_presentations_ = 0;
    size_t superseded_presentations_ = 0;
    size_t unchanged_presentations_ = 0;
    smooth_scroll scroll_;
    std::unordered_map<uint64_t, painted_frame> awaiting_presentation_;
    // Every paint's time by frame id, kept for the run: whether a discarded frame was replaced
    // within its refresh is known only once the next frame has painted.
    std::map<uint64_t, double> paint_times_;
    std::vector<double> sample_delivery_lag_ms_;
    std::vector<double> input_to_paint_ms_;
    std::vector<double> run_loop_busy_ms_;
    std::vector<double> run_loop_sleep_ms_;
    double woke_at_ = 0.0;
    double slept_at_ = 0.0;
    std::vector<double> tick_intervals_ms_;
    std::vector<double> paint_intervals_ms_;
    std::vector<double> presentation_intervals_ms_;
    std::vector<double> render_times_ms_;
    std::vector<double> paint_to_present_ms_;
    std::vector<double> target_to_present_ms_;
    std::vector<double> tick_delays_ms_;
    std::vector<double> input_to_present_ms_;
    std::vector<double> position_errors_;
    std::vector<double> presented_step_errors_;
    std::vector<double> motion_steps_;
    std::vector<double> events_per_paint_;
};

}  // namespace

int main(int argc, char** argv) {
    Options options;
    if (!parse_options(argc, argv, &options)) {
        usage(argv[0]);
        return 2;
    }

    std::vector<Sample> samples;
    if (options.replay_path) {
        std::string error;
        if (!read_trace(options.replay_path, &samples, &error)) {
            std::fprintf(stderr, "scroll_benchmark: %s\n", error.c_str());
            return 3;
        }
    } else if (std::ifstream(options.record_path)) {
        std::fprintf(stderr, "scroll_benchmark: refusing to overwrite %s\n", options.record_path);
        return 2;
    } else {
        // Live events carry their own timestamps; nothing is phased against the ticks.
        options.input_phase_ms = 0.0;
    }

    std::setvbuf(stdout, nullptr, _IONBF, 0);
    px_init("scroll-benchmark", "com.example.scroll-benchmark", argc, argv, 0);

    ScrollBenchmark benchmark(options, std::move(samples));
    px_window_t* window =
        px_create_window(&benchmark, nullptr, kWindowWidth, kWindowHeight, "scroll benchmark",
                         kWindowBackground, PX_WINDOW_DEFAULT);
    if (!window) {
        std::fprintf(stderr, "scroll_benchmark: failed to create window\n");
        return 1;
    }
    benchmark.attach(window);
    px_set_application_event_handler(&benchmark);
    if (options.record_path) {
        std::printf("scroll the document; Escape writes %s and quits\n", options.record_path);
    }
    px_show_window(window);
    px_set_animating(window, true);
    if (options.full_screen) {
        // After show: AppKit only animates into full screen for a window that is already on
        // screen.
        px_set_full_screen(window, true);
    }
    px_mark_dirty(window);
    px_run_event_loop();
    benchmark.write_recording();
    px_destroy_window(window);
    return 0;
}
