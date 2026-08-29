#include "experiments/platform/px/px.h"
#include "experiments/platform/px/px_font_private.h"
#include "experiments/platform/ui/window.h"

#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <print>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::array<std::string_view, 4> kSectionTitles = {
    "Rendering",
    "Layout and hit testing",
    "Syntax highlighting",
    "Compositor",
};

constexpr std::array<std::string_view, 10> kSourceLines = {
    "void render_frame(EditorView& view) {",
    "    const auto visible = view.visible_region();",
    "    for (const auto& line : visible.lines()) {",
    "        syntax.draw_line(context, line);",
    "        selections.paint(context, line);",
    "        carets.paint(context, line);",
    "    }",
    "    compositor.present(dirty_regions);",
    "}",
    "render_context.flush();",
};

constexpr std::array<std::string_view, 8> kDetailLines = {
    "CoreText shaping · cached glyph coverage · OpenGL atlas",
    "Only damaged regions are repainted before presentation",
    "Ligatures: fi ffi  ·  operators: != <= -> =>",
    "Fallback: 你好  привет  مرحبا  👋",
    "Selection geometry follows shaped glyph advances",
    "The renderer clips every draw to the document viewport",
    "Glyph masks are uploaded lazily and reused across frames",
    "Presentation stays synchronized with the display refresh",
};

struct VisibleRow {
    int row;
    double y;
    bool section;
};

enum class ScrollMode { kNormal, kOnscreen, kLower, kOffscreen };
enum class ContentMode { kAll, kAllCached, kRectangles, kText, kTextCached };

constexpr double kDocumentHeight = 5200.0;
constexpr double kBarSpeed = 720.0;
constexpr double kBarWidth = 120.0;
constexpr double kBarHeight = 10.0;

double initial_scroll_offset(ScrollMode mode) {
    switch (mode) {
    case ScrollMode::kLower:
        return -300.0;
    case ScrollMode::kOffscreen:
        return 1'000'000.0;
    case ScrollMode::kNormal:
    case ScrollMode::kOnscreen:
        return 0.0;
    }
    return 0.0;
}

using LayoutBatches = std::vector<fx_layout_batch>;

LayoutBatches shape_text(px_font_t* font, std::string_view text) {
    return shape_text_buffer_batches(font, text);
}

void draw_layout_batches(px_render_context* context,
                         px_font_t* font,
                         vec2 position,
                         fcolor color,
                         LayoutBatches* batches) {
    for (fx_layout_batch& batch : *batches) {
        context->draw_shaped_text(font, vec2{position.x + batch.x_offset, position.y}, color,
                                  &batch.layout, true);
    }
}

class DemoControl final : public control, public px_input_client {
public:
    DemoControl(window* window,
                px_font_t* body_font,
                px_font_t* detail_font,
                ScrollMode scroll_mode,
                ContentMode content_mode,
                double expected_scroll_distance)
        : window_(window),
          body_font_(body_font),
          detail_font_(detail_font),
          scroll_mode_(scroll_mode),
          content_mode_(content_mode),
          scroll_offset_(initial_scroll_offset(scroll_mode)),
          expected_scroll_distance_(expected_scroll_distance) {
        if (content_mode_ == ContentMode::kAllCached ||
            content_mode_ == ContentMode::kTextCached) {
            for (size_t i = 0; i < kSectionTitles.size(); ++i) {
                section_layouts_[i] = shape_text(body_font_, kSectionTitles[i]);
            }
            for (size_t i = 0; i < kSourceLines.size(); ++i) {
                source_layouts_[i] = shape_text(body_font_, kSourceLines[i]);
            }
            for (size_t i = 0; i < kDetailLines.size(); ++i) {
                detail_layouts_[i] = shape_text(detail_font_, kDetailLines[i]);
            }
        }
    }

    void set_bar_distance(double distance) { bar_distance_ = distance; }

    bool handle_event(const px_event_t* event) override {
        switch (event->type) {
        case PX_EVENT_KEY:
            if (event->pressed && event->key == PX_KEY_ESCAPE) {
                window_->close();
                return true;
            }
            break;

        case PX_EVENT_SCROLL:
            ++scroll_event_count_;
            scroll_distance_ += std::abs(event->scroll_delta.y);
            if (!reported_scroll_input_ && scroll_distance_ >= expected_scroll_distance_) {
                reported_scroll_input_ = true;
                std::printf("scroll_input_complete events=%d distance=%.0f\n", scroll_event_count_,
                            scroll_distance_);
            }
            if (scroll_mode_ == ScrollMode::kNormal) {
                scroll_offset_ = std::clamp(scroll_offset_ - event->scroll_delta.y, 0.0,
                                            kDocumentHeight - 120.0);
            }
            window_->mark_dirty();
            break;

        default:
            break;
        }
        return false;
    }

    void draw(px_render_context* context,
              rect bounds,
              const rect* dirty,
              int dirty_count) override {
        (void)dirty;
        (void)dirty_count;
        std::vector<VisibleRow> visible_rows;
        visible_rows.reserve(static_cast<size_t>(bounds.h / 64.0) + 3);

        context->begin_rect_batch();
        context->draw_rect(bounds, fcolor{0.09f, 0.10f, 0.12f, 1.0f});
        context->end_rect_batch();

        const double content_top = bounds.h * 0.25;
        context->push_state(false);
        context->restrict_clip_rect(rect{0.0, content_top, bounds.w, bounds.h - content_top});

        const double document_y = content_top - scroll_offset_;
        const double document_width = std::max(280.0, bounds.w - 128.0);
        const bool draw_rectangles = content_mode_ != ContentMode::kText &&
                                     content_mode_ != ContentMode::kTextCached;
        if (draw_rectangles) {
            context->begin_rect_batch();
            context->draw_rect(rect{48.0, document_y, document_width, kDocumentHeight},
                               fcolor{0.105f, 0.115f, 0.135f, 1.0f});
        }

        constexpr double kRowPitch = 64.0;
        constexpr double kRowHeight = 48.0;
        constexpr int kRowCount = static_cast<int>(kDocumentHeight / kRowPitch);
        for (int row = 0; row < kRowCount; ++row) {
            const double y = document_y + 24.0 + row * kRowPitch;
            if (y + kRowHeight < content_top - kRowPitch || y > bounds.h + kRowPitch) {
                continue;
            }

            const bool section = row % 8 == 0;
            if (draw_rectangles) {
                const float shade = row % 2 == 0 ? 0.175f : 0.145f;
                context->draw_rect(rect{68.0, y, document_width - 40.0, kRowHeight},
                                   fcolor{shade, shade + 0.008f, shade + 0.025f, 1.0f});
                context->draw_rect(rect{68.0, y, section ? 7.0 : 3.0, kRowHeight},
                                   section ? fcolor{0.30f, 0.68f, 0.93f, 1.0f}
                                           : fcolor{0.31f, 0.36f, 0.43f, 1.0f});
                context->draw_rect(rect{88.0, y + 39.0, document_width - 72.0, 1.0},
                                   fcolor{0.235f, 0.255f, 0.295f, 1.0f});

                const float marker_r = 0.36f + static_cast<float>((row * 29) % 25) / 100.0f;
                const float marker_g = 0.40f + static_cast<float>((row * 17) % 22) / 100.0f;
                context->draw_rect(rect{document_width + 18.0, y + 15.0, 10.0, 18.0},
                                   fcolor{marker_r, marker_g, 0.72f, 1.0f});
            }
            visible_rows.push_back(VisibleRow{row, y, section});
        }
        if (draw_rectangles) {
            context->end_rect_batch();
        }

        if (content_mode_ != ContentMode::kRectangles && body_font_ && detail_font_) {
            const bool cached = content_mode_ == ContentMode::kAllCached ||
                                content_mode_ == ContentMode::kTextCached;
            for (const VisibleRow& visible : visible_rows) {
                const size_t title_index = visible.section
                                               ? (visible.row / 8) % kSectionTitles.size()
                                               : visible.row % kSourceLines.size();
                const size_t detail_index = visible.row % kDetailLines.size();
                const fcolor title_color = visible.section
                                               ? fcolor{0.70f, 0.84f, 0.96f, 1.0f}
                                               : fcolor{0.63f, 0.68f, 0.76f, 1.0f};
                if (cached) {
                    LayoutBatches* title_layouts = visible.section
                                                               ? &section_layouts_[title_index]
                                                               : &source_layouts_[title_index];
                    draw_layout_batches(context, body_font_, vec2{88.0, visible.y + 19.0},
                                        title_color, title_layouts);
                    draw_layout_batches(context, detail_font_, vec2{88.0, visible.y + 34.0},
                                        fcolor{0.39f, 0.45f, 0.54f, 1.0f},
                                        &detail_layouts_[detail_index]);
                } else {
                    const std::string_view title = visible.section
                                                       ? kSectionTitles[title_index]
                                                       : kSourceLines[title_index];
                    LayoutBatches title_layouts = shape_text(body_font_, title);
                    LayoutBatches detail_layouts =
                        shape_text(detail_font_, kDetailLines[detail_index]);
                    draw_layout_batches(context, body_font_, vec2{88.0, visible.y + 19.0},
                                        title_color, &title_layouts);
                    draw_layout_batches(context, detail_font_, vec2{88.0, visible.y + 34.0},
                                        fcolor{0.39f, 0.45f, 0.54f, 1.0f}, &detail_layouts);
                }
            }
        }
        context->pop_state();

        context->begin_rect_batch();
        const double travel = std::max(1.0, bounds.w - kBarWidth);
        const double bar_x = std::fmod(bar_distance_, travel);
        context->draw_rect(rect{bar_x, 12.0, kBarWidth, kBarHeight},
                           fcolor{0.20f, 0.90f, 0.45f, 1.0f});
        context->end_rect_batch();
    }

    void insert_text(const char* utf8, px_range_t replacement) override {
        std::println("[ime] insert_text \"{}\" replacing [{},{})", utf8, replacement.location,
                     replacement.length);
        committed_ += utf8;
        marked_.clear();
        window_->mark_dirty();
    }

    void set_marked_text(const char* utf8, px_range_t selected, px_range_t replacement) override {
        marked_ = utf8;
        std::println("[ime] marked \"{}\"", marked_);
    }

    void unmark_text() override { marked_.clear(); }
    bool has_marked_text() const override { return !marked_.empty(); }

    px_range_t marked_range() const override {
        if (marked_.empty()) {
            return px_range_t::none();
        }
        return px_range_t{static_cast<int64_t>(committed_.size()),
                          static_cast<int64_t>(marked_.size())};
    }

    px_range_t selected_range() const override {
        return px_range_t{static_cast<int64_t>(committed_.size()), 0};
    }

    rect first_rect_for_range(px_range_t range, px_range_t* actual) override {
        if (actual) {
            *actual = px_range_t{0, 0};
        }
        return rect{12.0, 40.0, 1.0, 18.0};
    }

    int64_t character_index_for_point(vec2 pos) override { return 0; }

    void do_command(const char* selector_name) override {
        std::println("[ime] do_command {}", selector_name);
    }

private:
    window* window_ = nullptr;
    px_font_t* body_font_ = nullptr;
    px_font_t* detail_font_ = nullptr;
    double bar_distance_ = 0.0;
    ScrollMode scroll_mode_ = ScrollMode::kNormal;
    ContentMode content_mode_ = ContentMode::kAll;
    double scroll_offset_ = 0.0;
    int scroll_event_count_ = 0;
    double scroll_distance_ = 0.0;
    double expected_scroll_distance_ = 640.0 * 12.0;
    bool reported_scroll_input_ = false;
    std::array<LayoutBatches, kSectionTitles.size()> section_layouts_;
    std::array<LayoutBatches, kSourceLines.size()> source_layouts_;
    std::array<LayoutBatches, kDetailLines.size()> detail_layouts_;
    std::string committed_;
    std::string marked_;
};

class DemoWindow final : public window_impl {
public:
    using window_impl::window_impl;

    void set_control(DemoControl* control) { control_ = control; }

    void animation_tick(double now) override {
        if (!control_) {
            return;
        }
        if (start_time_ == 0.0) {
            start_time_ = now;
        }
        control_->set_bar_distance((now - start_time_) * kBarSpeed);
        mark_dirty();
    }

private:
    DemoControl* control_ = nullptr;
    double start_time_ = 0.0;
};

class DemoApp final : public px_application_event_handler {
public:
    void appearance_changed() override {
        std::println("[app] appearance_changed dark={}", px_os_in_dark_mode() ? 1 : 0);
    }
};

}  // namespace

int main(int argc, char** argv) {
    ScrollMode scroll_mode = ScrollMode::kNormal;
    if (argc >= 2) {
        const std::string_view mode = argv[1];
        if (mode == "onscreen") {
            scroll_mode = ScrollMode::kOnscreen;
        } else if (mode == "lower") {
            scroll_mode = ScrollMode::kLower;
        } else if (mode == "offscreen") {
            scroll_mode = ScrollMode::kOffscreen;
        } else if (mode != "normal") {
            std::println(stderr,
                         "usage: {} [normal|onscreen|lower|offscreen] [small|large]", argv[0]);
            return 2;
        }
    }
    bool large = false;
    if (argc >= 3) {
        const std::string_view size = argv[2];
        if (size == "large") {
            large = true;
        } else if (size != "small") {
            std::println(stderr,
                         "usage: {} [normal|onscreen|lower|offscreen] [small|large]", argv[0]);
            return 2;
        }
    }
    double expected_scroll_distance = 640.0 * 12.0;
    if (argc >= 4) {
        char* end = nullptr;
        expected_scroll_distance = std::strtod(argv[3], &end);
        if (!end || *end != '\0' || expected_scroll_distance <= 0.0) {
            std::println(
                stderr,
                "usage: {} [normal|onscreen|lower|offscreen] [small|large] [scroll_distance]",
                argv[0]);
            return 2;
        }
    }
    ContentMode content_mode = ContentMode::kAll;
    if (argc >= 5) {
        const std::string_view content = argv[4];
        if (content == "all-cached") {
            content_mode = ContentMode::kAllCached;
        } else if (content == "rectangles") {
            content_mode = ContentMode::kRectangles;
        } else if (content == "text-cached") {
            content_mode = ContentMode::kTextCached;
        } else if (content == "text") {
            content_mode = ContentMode::kText;
        } else if (content != "all") {
            std::println(
                stderr,
                "usage: {} [normal|onscreen|lower|offscreen] [small|large] [scroll_distance] [all|all-cached|rectangles|text|text-cached]",
                argv[0]);
            return 2;
        }
    }
    if (argc > 5) {
        std::println(
            stderr,
            "usage: {} [normal|onscreen|lower|offscreen] [small|large] [scroll_distance] [all|all-cached|rectangles|text|text-cached]",
            argv[0]);
        return 2;
    }

    std::setvbuf(stdout, nullptr, _IONBF, 0);
    px_init("platform_demo_isolation", "com.example.platform-demo-isolation", argc, argv, 0);

    DemoApp app;
    px_set_application_event_handler(&app);

    DemoWindow window(large ? 1400.0 : 900.0, large ? 800.0 : 600.0, "platform demo isolation",
                      fcolor{0.09f, 0.10f, 0.12f, 1.0f});
    window_basic_aspect basic(&window);
    window_hover_aspect hover(&window);
    window.add_window_aspect(&basic);
    window.add_window_aspect(&hover);

    px_font_t* body_font = px_create_font("Menlo", 15.0f);
    px_font_t* detail_font = px_create_font("Menlo", 12.0f);
    DemoControl root(&window, body_font, detail_font, scroll_mode, content_mode,
                     expected_scroll_distance);
    window.set_control(&root);
    window.set_root_control(&root);
    window.set_input_client(&root);

    window.show();
    const vec2 origin = px_window_position(window.px_window());
    const vec2 size = px_window_size(window.px_window());
    std::printf("capture_rect=%.0f,%.0f,%.0f,%.0f\n", origin.x, origin.y, size.x, size.y);
    std::printf("bar_speed=%.0f points/s\n", kBarSpeed);
    px_run_event_loop();
    return 0;
}
