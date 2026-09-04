#include "experiments/platform/px/px.h"
#include "experiments/platform/ui/retained_text.h"
#include "experiments/platform/smoothness/scroll_trace.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
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

struct Options {
    const char* trace_path = nullptr;
    int repetitions = 1;
    bool dump_frames = false;
    bool keep_open = false;
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
    std::fprintf(stderr, "usage: %s TRACE.tsv [--repetitions N] [--dump-frames] [--keep-open]\n",
                 program);
}

bool parse_options(int argc, char** argv, Options* options) {
    if (argc < 2) {
        return false;
    }
    if (argc == 2 && std::strcmp(argv[1], "--help") == 0) {
        usage(argv[0]);
        std::exit(0);
    }
    options->trace_path = argv[1];
    for (int i = 2; i < argc; ++i) {
        if (std::strcmp(argv[i], "--repetitions") == 0 && i + 1 < argc) {
            if (!parse_positive_int(argv[++i], 100, &options->repetitions)) {
                return false;
            }
        } else if (std::strcmp(argv[i], "--dump-frames") == 0) {
            options->dump_frames = true;
        } else if (std::strcmp(argv[i], "--keep-open") == 0) {
            options->keep_open = true;
        } else {
            return false;
        }
    }
    return true;
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

class ScrollBenchmark final : public px_window_event_handler {
public:
    ScrollBenchmark(Options options, std::vector<scroll_trace::Sample> samples)
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

        for (const scroll_trace::Sample& sample : samples_) {
            input_distance_ += std::abs(sample.scrolling_delta_y);
        }
    }

    void attach(px_window_t* window) { window_ = window; }

    bool handle_event(px_event_t* event) override {
        if (event->type == PX_EVENT_KEY && event->pressed && event->key == PX_KEY_ESCAPE) {
            px_close_window(window_);
            return true;
        }
        return false;
    }

    void animation_tick(double now) override {
        if (first_tick_time_ == 0.0) {
            first_tick_time_ = now;
            px_mark_dirty(window_);
            return;
        }
        if (playback_start_time_ == 0.0) {
            if (now - first_tick_time_ < kWarmupSeconds) {
                px_mark_dirty(window_);
                return;
            }
            playback_start_time_ = now;
            previous_tick_time_ = now;
            previous_rendered_offset_ = scroll_offset_;
            std::printf("playback_started target_time=%.6f\n", now);
        } else if (next_sample_ < samples_.size()) {
            tick_intervals_ms_.push_back((now - previous_tick_time_) * 1000.0);
            previous_tick_time_ = now;
        }

        const double elapsed = std::max(0.0, now - playback_start_time_);
        const uint64_t elapsed_ns = static_cast<uint64_t>(elapsed * 1'000'000'000.0);
        size_t events = 0;
        double delta_y = 0.0;
        while (next_sample_ < samples_.size() && samples_[next_sample_].time_ns <= elapsed_ns) {
            delta_y += samples_[next_sample_].scrolling_delta_y;
            ++next_sample_;
            ++events;
        }

        if (events != 0) {
            ++input_ticks_;
            pending_events_ += events;
            if (delta_y != 0.0) {
                ++motion_ticks_;
                ++pending_motion_ticks_;
                scroll_offset_ -= delta_y;
            }
            px_mark_dirty(window_);
        }

        const double trace_end = static_cast<double>(samples_.back().time_ns) / 1'000'000'000.0;
        if (!reported_ && next_sample_ == samples_.size() &&
            elapsed >= trace_end + kSettleSeconds) {
            report();
            reported_ = true;
            if (!options_.keep_open) {
                px_set_timeout([window = window_] { px_close_window(window); }, 0);
            }
        }
    }

    void paint(px_render_context* context,
               rect bounds,
               const rect* dirty,
               int dirty_count) override {
        if (!backend_) {
            backend_ = context->supports_batching() ? "opengl" : "skia";
        }

        const auto begin = std::chrono::steady_clock::now();
        draw_scene(context, bounds);
        const auto end = std::chrono::steady_clock::now();

        if (playback_start_time_ == 0.0 || pending_events_ == 0) {
            return;
        }

        const double paint_time = px_now();
        if (previous_paint_time_ != 0.0) {
            paint_intervals_ms_.push_back((paint_time - previous_paint_time_) * 1000.0);
        }
        previous_paint_time_ = paint_time;
        render_times_ms_.push_back(std::chrono::duration<double, std::milli>(end - begin).count());
        events_per_paint_.push_back(static_cast<double>(pending_events_));

        const double motion_step = std::abs(scroll_offset_ - previous_rendered_offset_);
        if (motion_step != 0.0) {
            motion_steps_.push_back(motion_step);
            displayed_distance_ += motion_step;
            previous_rendered_offset_ = scroll_offset_;
        }
        if (pending_motion_ticks_ > 1) {
            coalesced_motion_ticks_ += pending_motion_ticks_ - 1;
        }
        if (options_.dump_frames) {
            std::printf("FRAME time=%.6f offset=%.3f events=%zu motion_ticks=%zu render=%.3fms\n",
                        paint_time - playback_start_time_, scroll_offset_, pending_events_,
                        pending_motion_ticks_, render_times_ms_.back());
        }
        pending_events_ = 0;
        pending_motion_ticks_ = 0;
        ++paint_count_;
    }

private:
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
        const int64_t first_line = static_cast<int64_t>(std::floor(scroll_offset_ / kLineHeight));
        const double fractional_scroll = scroll_offset_ - first_line * kLineHeight;

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
        const double thumb_progress = std::fmod(std::abs(scroll_offset_), 4000.0) / 4000.0;
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
        const double duration_ms = static_cast<double>(samples_.back().time_ns) / 1'000'000.0;
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

        std::printf("benchmark=scroll backend=%s trace=%s samples=%zu trace_duration=%.3fms "
                    "repetitions=%d input_distance=%.3fpt\n",
                    backend_ ? backend_ : "unknown", options_.trace_path, samples_.size(),
                    duration_ms, options_.repetitions, input_distance_);
        print_distribution("display_tick_interval", "ms", tick_intervals_ms_);
        print_distribution("paint_interval", "ms", paint_intervals_ms_);
        print_distribution("render_submit", "ms", render_times_ms_);
        print_distribution("motion_step", "pt", motion_steps_);
        print_distribution("events_per_paint", "", events_per_paint_);
        std::printf("summary input_ticks=%zu motion_ticks=%zu paints=%zu missed_display_ticks=%zu "
                    "coalesced_motion_ticks=%zu pending_events=%zu displayed_distance=%.3fpt "
                    "distance_ratio=%.6f\n",
                    input_ticks_, motion_ticks_, paint_count_, missed_display_ticks,
                    coalesced_motion_ticks_, pending_events_, displayed_distance_,
                    input_distance_ == 0.0 ? 1.0 : displayed_distance_ / input_distance_);
    }

    Options options_;
    std::vector<scroll_trace::Sample> samples_;
    px_window_t* window_ = nullptr;
    px_font_t* body_font_ = nullptr;
    px_font_t* ui_font_ = nullptr;
    px_font_t* heading_font_ = nullptr;
    std::vector<PreparedLine> lines_;
    std::vector<PreparedText> sidebar_;
    std::vector<PreparedText> line_numbers_;
    const char* backend_ = nullptr;
    size_t next_sample_ = 0;
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
    double scroll_offset_ = 0.0;
    double previous_rendered_offset_ = 0.0;
    double input_distance_ = 0.0;
    double displayed_distance_ = 0.0;
    bool reported_ = false;
    std::vector<double> tick_intervals_ms_;
    std::vector<double> paint_intervals_ms_;
    std::vector<double> render_times_ms_;
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

    std::vector<scroll_trace::Sample> samples;
    std::string error;
    if (!scroll_trace::read_trace(options.trace_path, &samples, &error)) {
        std::fprintf(stderr, "scroll_benchmark: %s\n", error.c_str());
        return 3;
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
    px_show_window(window);
    px_mark_dirty(window);
    px_run_event_loop();
    px_destroy_window(window);
    return 0;
}
