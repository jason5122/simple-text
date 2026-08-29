#include "experiments/platform/px/px.h"
#include "experiments/platform/px/px_font_private.h"
#include "experiments/platform/ui/window.h"
#include <array>
#include <cmath>
#include <print>
#include <string>
#include <string_view>
#include <vector>

namespace {

constexpr std::string_view kHeaderText = "fx / CoreText — fi != -> 你好 مرحبا 👋";

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

using LayoutBatches = std::vector<fx_layout_batch>;

void draw_cached_text(px_render_context* context,
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
    DemoControl(window* w,
                const window_hover_aspect* hover,
                px_font_t* body_font,
                px_font_t* detail_font)
        : window_(w), hover_(hover), body_font_(body_font), detail_font_(detail_font) {
        header_layout_ = shape_text_buffer_batches(body_font_, kHeaderText);
        for (size_t i = 0; i < kSectionTitles.size(); ++i) {
            section_layouts_[i] = shape_text_buffer_batches(body_font_, kSectionTitles[i]);
        }
        for (size_t i = 0; i < kSourceLines.size(); ++i) {
            source_layouts_[i] = shape_text_buffer_batches(body_font_, kSourceLines[i]);
        }
        for (size_t i = 0; i < kDetailLines.size(); ++i) {
            detail_layouts_[i] = shape_text_buffer_batches(detail_font_, kDetailLines[i]);
        }
    }

    void set_phase(double phase) { phase_ = phase; }

    bool handle_event(const px_event_t* event) override {
        switch (event->type) {
        case PX_EVENT_KEY:
            if (event->pressed && event->key == PX_KEY_ESCAPE) {
                window_->close();
                return true;
            }
            break;

        case PX_EVENT_SCROLL:
            scroll_offset_ =
                std::clamp(scroll_offset_ - event->scroll_delta.y, 0.0, kDocumentHeight - 120.0);
            window_->mark_dirty();
            break;

        case PX_EVENT_MOUSE_MOTION:
            mark_hover_square_dirty(event->pos);
            break;

        case PX_EVENT_MOUSE_LEAVE:
            mark_hover_square_dirty(last_hover_pos_);
            break;

        default:
            break;
        }
        return false;
    }

    void draw(px_render_context* rc, rect bounds, const rect* dirty, int dirty_count) override {
        std::vector<VisibleRow> visible_rows;
        visible_rows.reserve(static_cast<size_t>(bounds.h / 64.0) + 3);

        rc->begin_rect_batch();
        // The context's dirty stencil means this full-window background touches only invalid
        // pixels.
        rc->draw_rect(bounds, fcolor{0.09f, 0.10f, 0.12f, 1.0f});

        // A grid, so resizing and dirty-rect behaviour is visible.
        constexpr double kCell = 48.0;
        for (double y = 0; y < bounds.h; y += kCell) {
            for (double x = 0; x < bounds.w; x += kCell) {
                const bool alternate = static_cast<int>(x / kCell + y / kCell) % 2 == 0;
                const float shade = alternate ? 0.14f : 0.17f;
                rc->draw_rect(rect{x + 1, y + 1, kCell - 2, kCell - 2},
                              fcolor{shade, shade, shade + 0.02f, 1});
            }
        }

        const double document_y = 72.0 - scroll_offset_;
        const double document_width = std::max(280.0, bounds.w - 128.0);
        rc->draw_rect(rect{48.0, document_y, document_width, kDocumentHeight},
                      fcolor{0.105f, 0.115f, 0.135f, 1.0f});

        constexpr double kRowPitch = 64.0;
        constexpr double kRowHeight = 48.0;
        constexpr int kRowCount = static_cast<int>(kDocumentHeight / kRowPitch);
        for (int row = 0; row < kRowCount; ++row) {
            const double y = document_y + 24.0 + row * kRowPitch;
            if (y + kRowHeight < -kRowPitch || y > bounds.h + kRowPitch) {
                continue;
            }

            const bool section = row % 8 == 0;
            const float shade = row % 2 == 0 ? 0.175f : 0.145f;
            rc->draw_rect(rect{68.0, y, document_width - 40.0, kRowHeight},
                          fcolor{shade, shade + 0.008f, shade + 0.025f, 1.0f});
            rc->draw_rect(rect{68.0, y, section ? 7.0 : 3.0, kRowHeight},
                          section ? fcolor{0.30f, 0.68f, 0.93f, 1.0f}
                                  : fcolor{0.31f, 0.36f, 0.43f, 1.0f});

            rc->draw_rect(rect{88.0, y + 39.0, document_width - 72.0, 1.0},
                          fcolor{0.235f, 0.255f, 0.295f, 1.0f});

            const float marker_r = 0.36f + static_cast<float>((row * 29) % 25) / 100.0f;
            const float marker_g = 0.40f + static_cast<float>((row * 17) % 22) / 100.0f;
            rc->draw_rect(rect{document_width + 18.0, y + 15.0, 10.0, 18.0},
                          fcolor{marker_r, marker_g, 0.72f, 1.0f});
            visible_rows.push_back(VisibleRow{row, y, section});
        }

        rc->end_rect_batch();

        // Retain shaped layouts as an editor does, while the renderer continues to resolve their
        // glyphs through fx_glyph_cache and the GL atlas on every paint.
        if (body_font_) {
            draw_cached_text(rc, body_font_, vec2{24.0, 48.0},
                             fcolor{0.78f, 0.86f, 0.96f, 1.0f}, &header_layout_);
        }

        if (body_font_ && detail_font_) {
            rc->push_state(false);
            rc->restrict_clip_rect(rect{68.0, document_y, document_width - 40.0,
                                        std::max(0.0, bounds.h - document_y)});
            for (const VisibleRow& visible : visible_rows) {
                const size_t title_index = visible.section
                                               ? (visible.row / 8) % kSectionTitles.size()
                                               : visible.row % kSourceLines.size();
                LayoutBatches* title_layouts = visible.section
                                                   ? &section_layouts_[title_index]
                                                   : &source_layouts_[title_index];
                draw_cached_text(rc, body_font_, vec2{88.0, visible.y + 19.0},
                                 visible.section ? fcolor{0.70f, 0.84f, 0.96f, 1.0f}
                                                 : fcolor{0.63f, 0.68f, 0.76f, 1.0f},
                                 title_layouts);
                draw_cached_text(rc, detail_font_, vec2{88.0, visible.y + 34.0},
                                 fcolor{0.39f, 0.45f, 0.54f, 1.0f},
                                 &detail_layouts_[visible.row % kDetailLines.size()]);
            }
            rc->pop_state();
        }

        // Keep interactive overlays above both the document rectangles and its text.
        rc->begin_rect_batch();
        const double sweep = (std::sin(phase_) * 0.5 + 0.5) * std::max(0.0, bounds.w - 120.0);
        rc->draw_rect(rect{sweep, 12.0, 120.0, 10.0}, fcolor{0.35f, 0.65f, 0.95f, 1.0f});

        if (hover_ && hover_->inside()) {
            const vec2 p = hover_->pos();
            rc->draw_rect(rect{p.x - 10, p.y - 10, 20, 20}, fcolor{0.95f, 0.55f, 0.25f, 1.0f});
        }
        rc->end_rect_batch();
    }

    // ─────────────────────────────────────────────────────────────────────────────────────────────
    // px_input_client
    // ─────────────────────────────────────────────────────────────────────────────────────────────
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
        // Where the candidate window should appear.
        return rect{12.0, 40.0, 1.0, 18.0};
    }

    int64_t character_index_for_point(vec2 pos) override { return 0; }

    void do_command(const char* selector_name) override {
        std::println("[ime] do_command {}", selector_name);
    }

private:
    // Covers the square drawn at hover_->pos() (see draw()). Marking only the old and new
    // footprint, rather than the whole window, is what keeps a mouse-move sample cheap -- the same
    // division of labor drag_resizer relies on: touch just the pixels that actually changed, and
    // let the dirty-rect scissor in draw() do the rest. Both rects are needed: the new one so the
    // square appears at its current position, the old one so it's actually erased from its
    // previous one -- skip it and every past position keeps a stale copy of the square baked into
    // the framebuffer.
    void mark_hover_square_dirty(vec2 p) {
        constexpr double kHalf = 10.0;
        window_->mark_rect_dirty(
            rect{last_hover_pos_.x - kHalf, last_hover_pos_.y - kHalf, kHalf * 2, kHalf * 2});
        window_->mark_rect_dirty(rect{p.x - kHalf, p.y - kHalf, kHalf * 2, kHalf * 2});
        last_hover_pos_ = p;
    }

    window* window_ = nullptr;
    const window_hover_aspect* hover_ = nullptr;
    px_font_t* body_font_ = nullptr;
    px_font_t* detail_font_ = nullptr;
    static constexpr double kDocumentHeight = 5200.0;
    double phase_ = 0.0;
    double scroll_offset_ = 0.0;
    vec2 last_hover_pos_;
    LayoutBatches header_layout_;
    std::array<LayoutBatches, kSectionTitles.size()> section_layouts_;
    std::array<LayoutBatches, kSourceLines.size()> source_layouts_;
    std::array<LayoutBatches, kDetailLines.size()> detail_layouts_;
    std::string committed_;
    std::string marked_;
};

// A window_impl subclass only to route animation_tick into the control. ST does the equivalent by
// keeping a list of animating controls on window_impl and ticking them from the display link.
class DemoWindow final : public window_impl {
public:
    using window_impl::window_impl;

    void set_control(DemoControl* c) { control_ = c; }

    void animation_tick(double now) override {
        if (getenv("PX_NO_ANIMATION")) {
            return;
        }
        if (!control_) {
            return;
        }
        control_->set_phase(now * 1.5);
        mark_dirty();
    }

private:
    DemoControl* control_ = nullptr;
};

class DemoApp final : public px_application_event_handler {
public:
    void appearance_changed() override {
        std::println("[app] appearance_changed dark={}", px_os_in_dark_mode() ? 1 : 0);
    }
};

}  // namespace

int main(int argc, char** argv) {
    px_init("platform", "com.example.platform", argc, argv, 0);

    DemoApp app;
    px_set_application_event_handler(&app);

    DemoWindow win(900, 600, "platform - gl_render_context demo",
                   fcolor{0.09f, 0.10f, 0.12f, 1.0f});

    window_basic_aspect basic(&win);
    window_hover_aspect hover(&win);
    win.add_window_aspect(&basic);
    win.add_window_aspect(&hover);

    px_font_t* body_font = px_create_font("Menlo", 15.0f);
    px_font_t* detail_font = px_create_font("Menlo", 12.0f);
    DemoControl root(&win, &hover, body_font, detail_font);
    win.set_control(&root);
    win.set_root_control(&root);
    win.set_input_client(&root);

    win.show();
    px_run_event_loop();
    return 0;
}
