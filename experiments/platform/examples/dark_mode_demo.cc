#include "experiments/platform/px/px.h"
#include "experiments/platform/ui/retained_text.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr double kWindowWidth = 1400.0;
constexpr double kWindowHeight = 800.0;
constexpr double kSidebarWidth = 260.0;
constexpr double kGutterWidth = 64.0;
constexpr double kLineHeight = 20.0;
constexpr double kTextTop = 18.0;

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

size_t wrapped_index(int64_t index, size_t count) {
    const int64_t signed_count = static_cast<int64_t>(count);
    const int64_t remainder = index % signed_count;
    return static_cast<size_t>(remainder < 0 ? remainder + signed_count : remainder);
}

class DarkModeDemo final : public px_window_event_handler {
public:
    DarkModeDemo() {
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
    }

    void attach(px_window_t* window) { window_ = window; }

    bool handle_event(px_event_t* event) override {
        if (event->type == PX_EVENT_SCROLL) {
            scroll_offset_ -= event->scroll_delta.y;
            px_mark_dirty(window_);
            return true;
        }
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
        draw_scene(context, bounds);
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
        for (int row = -1; row < visible_rows; ++row) {
            const int64_t line = first_line + row;
            const double y = kTextTop + row * kLineHeight - fractional_scroll;
            if (line % 11 == 0) {
                context->draw_rect(
                    rect{document_left, y - 14.0, viewport.w - document_left, kLineHeight},
                    fcolor{0.105f, 0.135f, 0.185f, 0.32f});
            }
        }
        const double thumb_progress = std::fmod(std::abs(scroll_offset_), 4000.0) / 4000.0;
        context->draw_rect(rect{viewport.w - 8.0, thumb_progress * (viewport.h - 80.0), 5.0, 80.0},
                           fcolor{0.38f, 0.42f, 0.50f, 0.9f});
        context->end_rect_batch();

        context->begin_text_batch();
        for (int row = -1; row < visible_rows; ++row) {
            const int64_t line = first_line + row;
            const double y = kTextTop + row * kLineHeight - fractional_scroll;
            const size_t line_index = wrapped_index(line, lines_.size());
            const size_t number_index = wrapped_index(line, line_numbers_.size());
            draw_batches(context, body_font_, vec2{document_left + 10.0, y},
                         lines_[line_index].color, &lines_[line_index].text);
            draw_batches(context, ui_font_, vec2{kSidebarWidth + 12.0, y},
                         fcolor{0.48f, 0.50f, 0.56f, 1.0f}, &line_numbers_[number_index]);
        }
        context->end_text_batch();
        context->pop_state();
    }

    px_window_t* window_ = nullptr;
    px_font_t* body_font_ = nullptr;
    px_font_t* ui_font_ = nullptr;
    px_font_t* heading_font_ = nullptr;
    std::vector<PreparedLine> lines_;
    std::vector<PreparedText> sidebar_;
    std::vector<PreparedText> line_numbers_;
    double scroll_offset_ = 0.0;
};

}  // namespace

int main(int argc, char** argv) {
    px_init("dark-mode-demo", "com.example.dark-mode-demo", argc, argv, 0);

    DarkModeDemo demo;
    px_window_t* window = px_create_window(&demo, nullptr, kWindowWidth, kWindowHeight,
                                           "dark mode demo", kWindowBackground, PX_WINDOW_DEFAULT);
    if (!window) {
        return 1;
    }
    demo.attach(window);
    px_show_window(window);
    px_mark_dirty(window);
    px_run_event_loop();
    px_destroy_window(window);
    return 0;
}
