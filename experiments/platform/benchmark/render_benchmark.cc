// Deterministic renderer benchmark.
//
// The platform creates the active render context, but everything timed below is a fixed sequence
// of px_render_context operations followed by a backend synchronization. There is no synthetic
// input, display cadence, screen recording, or pixel tracking in the measurement.

#include "experiments/platform/px/px.h"
#include "experiments/platform/px/px_gl.h"
#include "experiments/platform/ui/retained_text.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
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
constexpr double kScrollStep = 0.375;
constexpr double kBudget120HzMs = 1000.0 / 120.0;
constexpr double kBudget60HzMs = 1000.0 / 60.0;

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
    "FOLDERS", "simple-text", "experiments", "platform",    "benchmark", "render_benchmark.cc",
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
    int repetitions = 12;
    int warmup = 8;
    int samples = 80;
    const char* preset = "extreme";
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
    std::fprintf(stderr,
                 "usage: %s [--preset extreme|realistic] [--repetitions N] "
                 "[--warmup N] [--samples N] [--keep-open]\n",
                 program);
}

bool parse_options(int argc, char** argv, Options* options) {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--preset") == 0 && i + 1 < argc) {
            const char* preset = argv[++i];
            if (std::strcmp(preset, "extreme") == 0) {
                options->preset = "extreme";
                options->repetitions = 12;
            } else if (std::strcmp(preset, "realistic") == 0) {
                options->preset = "realistic";
                options->repetitions = 1;
            } else {
                return false;
            }
        } else if (std::strcmp(argv[i], "--repetitions") == 0 && i + 1 < argc) {
            if (!parse_positive_int(argv[++i], 1000, &options->repetitions)) {
                return false;
            }
            options->preset = "custom";
        } else if (std::strcmp(argv[i], "--warmup") == 0 && i + 1 < argc) {
            if (!parse_positive_int(argv[++i], 10'000, &options->warmup)) {
                return false;
            }
        } else if (std::strcmp(argv[i], "--samples") == 0 && i + 1 < argc) {
            if (!parse_positive_int(argv[++i], 100'000, &options->samples)) {
                return false;
            }
        } else if (std::strcmp(argv[i], "--keep-open") == 0) {
            options->keep_open = true;
        } else if (std::strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            std::exit(0);
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

void print_distribution(const char* name, const std::vector<double>& samples) {
    const size_t over_120 = static_cast<size_t>(std::count_if(
        samples.begin(), samples.end(), [](double value) { return value > kBudget120HzMs; }));
    const size_t over_60 = static_cast<size_t>(std::count_if(
        samples.begin(), samples.end(), [](double value) { return value > kBudget60HzMs; }));
    std::printf("%s n=%zu p50=%.3fms p95=%.3fms p99=%.3fms max=%.3fms over_120hz=%zu "
                "over_60hz=%zu\n",
                name, samples.size(), quantile(samples, 0.50), quantile(samples, 0.95),
                quantile(samples, 0.99), *std::max_element(samples.begin(), samples.end()),
                over_120, over_60);
}

class RenderBenchmark final : public px_window_event_handler {
public:
    explicit RenderBenchmark(Options options) : options_(options) {
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
        line_numbers_.reserve(128);
        for (int i = 1; i <= 128; ++i) {
            line_numbers_.push_back(prepare_text(ui_font_, std::to_string(i)));
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

    void paint(px_render_context* context,
               rect bounds,
               const rect* dirty,
               int dirty_count) override {
        if (ran_) {
            return;
        }
        ran_ = true;

        const vec2 size = px_window_size(window_);
        const rect viewport{0.0, 0.0, size.x, size.y};
        const int visible_rows = static_cast<int>(std::ceil(size.y / kLineHeight)) + 2;
        const char* backend = context->supports_batching() ? "opengl" : "skia";

        std::printf("render_benchmark backend=%s preset=%s repetitions=%d warmup=%d samples=%d "
                    "viewport=%.0fx%.0f scale=%.2f rows=%d\n",
                    backend, options_.preset, options_.repetitions, options_.warmup,
                    options_.samples, size.x, size.y, context->dpi_scale_factor(), visible_rows);

        const double cold = measure(context, [&] { draw_full_scene(context, viewport, 0.0); });
        std::printf("cold_full %.3fms\n", cold);

        for (int i = 0; i < options_.warmup; ++i) {
            const double scroll = std::fmod(static_cast<double>(i) * kScrollStep, kLineHeight);
            draw_full_scene(context, viewport, scroll);
            if (context->supports_batching()) {
                glFinish();
            }
        }

        std::vector<double> full_samples;
        std::vector<double> scroll_samples;
        full_samples.reserve(static_cast<size_t>(options_.samples));
        scroll_samples.reserve(static_cast<size_t>(options_.samples));
        for (int i = 0; i < options_.samples; ++i) {
            const double scroll = std::fmod(static_cast<double>(i) * kScrollStep, kLineHeight);
            full_samples.push_back(
                measure(context, [&] { draw_full_scene(context, viewport, scroll); }));
        }
        for (int i = 0; i < options_.samples; ++i) {
            const double scroll =
                std::fmod(static_cast<double>(i + options_.samples) * kScrollStep, kLineHeight);
            scroll_samples.push_back(
                measure(context, [&] { draw_document(context, viewport, scroll); }));
        }

        print_distribution("warm_full", full_samples);
        print_distribution("warm_scroll", scroll_samples);
        std::fflush(stdout);

        if (!options_.keep_open) {
            px_set_timeout([window = window_] { px_close_window(window); }, 0);
        }
    }

private:
    template <typename Draw>
    static double measure(px_render_context* context, Draw&& draw) {
        const auto begin = std::chrono::steady_clock::now();
        std::forward<Draw>(draw)();
        if (context->supports_batching()) {
            glFinish();
        }
        const auto end = std::chrono::steady_clock::now();
        return std::chrono::duration<double, std::milli>(end - begin).count();
    }

    static void draw_batches(px_render_context* context,
                             px_font_t* font,
                             vec2 origin,
                             fcolor color,
                             PreparedText* text) {
        draw_retained_text(context, font, origin, color, text);
    }

    void draw_full_scene(px_render_context* context, rect viewport, double scroll) {
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

        draw_document(context, viewport, scroll);
    }

    void draw_document(px_render_context* context, rect viewport, double scroll) {
        const double document_left = kSidebarWidth + kGutterWidth;
        const rect document_clip{kSidebarWidth, 0.0, viewport.w - kSidebarWidth, viewport.h};
        const int visible_rows = static_cast<int>(std::ceil(viewport.h / kLineHeight)) + 2;

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
                const double y =
                    kTextTop + static_cast<double>(row) * kLineHeight - scroll + offset;
                if ((row + repetition) % 11 == 0) {
                    context->draw_rect(
                        rect{document_left, y - 14.0, viewport.w - document_left, kLineHeight},
                        fcolor{0.105f, 0.135f, 0.185f, 0.32f});
                }
                context->draw_rect(rect{document_left + 8.0 + (row * 37 + repetition * 19) % 180,
                                        y - 2.0, 22.0, 1.0},
                                   fcolor{0.30f, 0.40f, 0.56f, 0.45f});
            }
        }
        context->end_rect_batch();

        context->begin_text_batch();
        for (int repetition = 0; repetition < options_.repetitions; ++repetition) {
            const double offset = static_cast<double>(repetition % 4) * 0.25;
            const float alpha = repetition == 0 ? 1.0f : 0.22f;
            for (int row = -1; row < visible_rows; ++row) {
                const int positive_row = std::max(0, row);
                const size_t line_index = static_cast<size_t>(positive_row) % lines_.size();
                const size_t number_index =
                    static_cast<size_t>(positive_row) % line_numbers_.size();
                const double y =
                    kTextTop + static_cast<double>(row) * kLineHeight - scroll + offset;
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

    Options options_;
    px_window_t* window_ = nullptr;
    px_font_t* body_font_ = nullptr;
    px_font_t* ui_font_ = nullptr;
    px_font_t* heading_font_ = nullptr;
    std::vector<PreparedLine> lines_;
    std::vector<PreparedText> sidebar_;
    std::vector<PreparedText> line_numbers_;
    bool ran_ = false;
};

}  // namespace

int main(int argc, char** argv) {
    Options options;
    if (!parse_options(argc, argv, &options)) {
        usage(argv[0]);
        return 2;
    }

#if defined(_WIN32)
    _putenv_s("PX_NO_ANIMATION", "1");
#else
    setenv("PX_NO_ANIMATION", "1", 1);
#endif

    std::setvbuf(stdout, nullptr, _IONBF, 0);
    px_init("render-benchmark", "com.example.render-benchmark", argc, argv, 0);

    RenderBenchmark benchmark(options);
    px_window_t* window =
        px_create_window(&benchmark, nullptr, kWindowWidth, kWindowHeight, "render benchmark",
                         kWindowBackground, PX_WINDOW_DEFAULT);
    if (!window) {
        std::fprintf(stderr, "render_benchmark: failed to create window\n");
        return 1;
    }
    benchmark.attach(window);
    px_show_window(window);
    px_mark_dirty(window);
    px_run_event_loop();
    px_destroy_window(window);
    return 0;
}
