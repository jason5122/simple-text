#include "px/px.h"
#include "ui/retained_text.h"
#include "ui/smooth_scroll.h"
#include "ui/window.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <print>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

constexpr float kMainFontSize = 16.0f;
constexpr float kSidebarTitleFontSize = 13.0f;
constexpr float kSidebarFontSize = 12.0f;

constexpr double kSidebarWidth = 277.0;
constexpr double kSidebarTopPadding = 10.0;
constexpr double kSidebarLeftPadding = 16.0;
constexpr double kSidebarIndentWidth = 12.0;
constexpr double kSidebarIndentOffset = 5.0;
constexpr double kSidebarRowTopPadding = 3.0;
constexpr double kSidebarRowBottomPadding = 3.0;
constexpr size_t kSelectedSidebarLine = 1;

constexpr double kGutterWidth = 68.0;
constexpr double kMargin = 1.0;
constexpr double kGutterRightPadding = 20.0;
constexpr double kTextTop = 50.0;

constexpr double kScrollbarTop = 38.0;
constexpr double kScrollbarWidth = 10.0;
constexpr double kScrollbarMargin = 4.0;

constexpr double kMinimumThumbHeight = 36.0;
constexpr size_t kDocumentLineCount = 500;
// The find panel along the bottom, after Sublime's: a close mark, four option toggles, the query
// field with its match count, and the three action buttons hugging the right edge. Cmd+F shows
// it, Escape hides it. The document area ends where the panel begins.
constexpr double kFindPanelHeight = 40.0;
constexpr double kFindPanelPadding = 8.0;
constexpr double kFindItemHeight = 24.0;
constexpr double kFindItemGap = 6.0;
constexpr double kFindToggleWidth = 28.0;
constexpr double kFindButtonPadding = 12.0;
constexpr double kFindTextInset = 7.0;

// EDITOR_SCROLL_TRACE=1 logs scroll input, ticks, presented frames and marks (clicks) to stderr,
// for checking scroll smoothness against a real trackpad. benchmark/record_editor_scroll.sh
// wraps it.
const bool kScrollTrace = std::getenv("EDITOR_SCROLL_TRACE") != nullptr;

constexpr fcolor kFindPanelBackground{0.925f, 0.930f, 0.940f, 1.0f};
constexpr fcolor kFindPanelBorder{0.80f, 0.81f, 0.83f, 1.0f};
constexpr fcolor kFindInputBackground{1.0f, 1.0f, 1.0f, 1.0f};
constexpr fcolor kFindInputBorder{0.35f, 0.58f, 0.92f, 1.0f};
constexpr fcolor kFindButtonBackground{0.985f, 0.985f, 0.99f, 1.0f};
constexpr fcolor kFindButtonBorder{0.76f, 0.77f, 0.80f, 1.0f};
constexpr fcolor kFindToggleOnBackground{0.80f, 0.86f, 0.96f, 1.0f};
constexpr fcolor kFindLabelColor{0.22f, 0.23f, 0.26f, 1.0f};
constexpr fcolor kFindMutedColor{0.50f, 0.52f, 0.56f, 1.0f};

constexpr std::array<std::string_view, 4> kFindToggleLabels = {".*", "Aa", "ab", "↩"};
constexpr std::array<std::string_view, 3> kFindButtonLabels = {"Find", "Find Prev", "Find All"};

struct SidebarLine {
    std::string_view text;
    size_t indent_level = 0;
};

constexpr std::array<SidebarLine, 13> kSidebarLines = {
    SidebarLine{"FOLDERS", 0},
    SidebarLine{"User", 1},
    SidebarLine{"temp1", 2},
    SidebarLine{"temp2", 3},
    SidebarLine{"temp3", 4},
    SidebarLine{"ffi", 3},
    SidebarLine{"سلام", 3},
    SidebarLine{"buffer_demo.py", 2},
    SidebarLine{"Default.sublime-commands", 2},
    SidebarLine{"Default.sublime-theme", 2},
    SidebarLine{"Preferences.sublime-settings", 2},
    SidebarLine{"rasterizer_loop.py", 2},
    SidebarLine{"rasterizer_render.py", 2},
};

constexpr std::array<std::string_view, 32> kSourceLines = {
    "namespace editor {",
    "",
    "struct GlyphPosition {",
    "    uint32_t glyph_id = 0;",
    "    float x = 0.0f;",
    "    float advance = 0.0f;",
    "};",
    "",
    "class LineRenderer {",
    "public:",
    "    explicit LineRenderer(FontAtlas* atlas) : atlas_(atlas) {}",
    "",
    "    void draw_line(std::string_view text, vec2 origin) {",
    "        const ShapedLine& line = cache_.shape(text);",
    "        for (const GlyphPosition& glyph : line.glyphs()) {",
    "            const AtlasEntry entry = atlas_->lookup(glyph.glyph_id);",
    "            batch_.push(entry, origin.x + glyph.x, origin.y);",
    "        }",
    "        batch_.flush();",
    "    }",
    "",
    "private:",
    "    // Shaping and rasterization stay cached while scrolling.",
    "    ShapeCache cache_;",
    "    FontAtlas* atlas_ = nullptr;",
    "    GlyphBatch batch_;",
    "};",
    "",
    "constexpr double kLineHeight = 24.0;",
    "constexpr size_t kVisiblePadding = 2;",
    "",
    "}  // namespace editor",
};

using PreparedText = retained_text;

struct HighlightedRun {
    double x_offset = 0.0;
    fcolor color;
    PreparedText text;
};

using HighlightedLine = std::vector<HighlightedRun>;

constexpr fcolor kPlainText{0.23f, 0.24f, 0.27f, 1.0f};
constexpr fcolor kKeywordText{0.50f, 0.22f, 0.68f, 1.0f};
constexpr fcolor kTypeText{0.10f, 0.40f, 0.67f, 1.0f};
constexpr fcolor kFunctionText{0.13f, 0.46f, 0.55f, 1.0f};
constexpr fcolor kStringText{0.72f, 0.28f, 0.19f, 1.0f};
constexpr fcolor kNumberText{0.76f, 0.38f, 0.08f, 1.0f};
constexpr fcolor kCommentText{0.36f, 0.48f, 0.37f, 1.0f};

constexpr std::array<std::string_view, 11> kKeywords = {
    "class",   "const",   "constexpr", "explicit", "for",  "namespace",
    "nullptr", "private", "public",    "struct",   "void",
};

constexpr std::array<std::string_view, 14> kTypes = {
    "AtlasEntry", "FontAtlas",   "GlyphBatch", "GlyphPosition", "LineRenderer",
    "ShapeCache", "ShapedLine",  "double",     "float",         "size_t",
    "std",        "string_view", "uint32_t",   "vec2",
};

template <size_t Size>
bool contains_token(const std::array<std::string_view, Size>& tokens, std::string_view token) {
    return std::find(tokens.begin(), tokens.end(), token) != tokens.end();
}

bool is_identifier_start(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool is_identifier_continue(char c) { return is_identifier_start(c) || (c >= '0' && c <= '9'); }

HighlightedLine highlight_line(grapheme_shaper* shaper, std::string_view text) {
    HighlightedLine result;
    double x = 0.0;
    auto append = [&](std::string_view token, fcolor color) {
        PreparedText prepared = prepare_retained_text(shaper, token);
        result.push_back({.x_offset = x, .color = color, .text = std::move(prepared)});
        x += result.back().text.advance;
    };

    for (size_t start = 0; start < text.size();) {
        size_t end = start + 1;
        fcolor color = kPlainText;
        const char c = text[start];

        if (c == '/' && end < text.size() && text[end] == '/') {
            end = text.size();
            color = kCommentText;
        } else if (c == ' ' || c == '\t') {
            while (end < text.size() && (text[end] == ' ' || text[end] == '\t')) {
                ++end;
            }
        } else if (c == '"') {
            while (end < text.size()) {
                if (text[end] == '\\' && end + 1 < text.size()) {
                    end += 2;
                } else if (text[end++] == '"') {
                    break;
                }
            }
            color = kStringText;
        } else if (c >= '0' && c <= '9') {
            while (end < text.size() && ((text[end] >= '0' && text[end] <= '9') ||
                                         text[end] == '.' || text[end] == 'f')) {
                ++end;
            }
            color = kNumberText;
        } else if (is_identifier_start(c)) {
            while (end < text.size() && is_identifier_continue(text[end])) {
                ++end;
            }
            const std::string_view token = text.substr(start, end - start);
            size_t next = end;
            while (next < text.size() && text[next] == ' ') {
                ++next;
            }
            if (contains_token(kKeywords, token)) {
                color = kKeywordText;
            } else if (contains_token(kTypes, token)) {
                color = kTypeText;
            } else if (next < text.size() && text[next] == '(') {
                color = kFunctionText;
            }
        }

        append(text.substr(start, end - start), color);
        start = end;
    }
    return result;
}

void draw_layout(
    px_render_context* context, px_font_t* font, vec2 position, fcolor color, PreparedText* text) {
    draw_retained_text(context, font, position, color, text);
}

void draw_highlighted_line(px_render_context* context,
                           px_font_t* font,
                           vec2 position,
                           HighlightedLine* line) {
    for (HighlightedRun& run : *line) {
        draw_layout(context, font, vec2{position.x + run.x_offset, position.y}, run.color,
                    &run.text);
    }
}

class EditorControl final : public control {
public:
    EditorControl(window* window,
                  px_font_t* body_font,
                  px_font_t* sidebar_title_font,
                  px_font_t* sidebar_font,
                  px_font_t* gutter_font)
        : window_(window),
          body_font_(body_font),
          sidebar_title_font_(sidebar_title_font),
          sidebar_font_(sidebar_font),
          gutter_font_(gutter_font),
          line_height_(px_font_get_metrics(body_font).line_height),
          body_metrics_(px_font_get_metrics(body_font)),
          sidebar_title_metrics_(px_font_get_metrics(sidebar_title_font)),
          sidebar_metrics_(px_font_get_metrics(sidebar_font)) {
        grapheme_shaper* body_shaper = grapheme_shaper::instance(body_font_);
        grapheme_shaper* ui_shaper = grapheme_shaper::instance(sidebar_font_);
        find_close_layout_ = prepare_retained_text(ui_shaper, "×");
        for (size_t i = 0; i < kFindToggleLabels.size(); ++i) {
            find_toggle_layouts_[i] = prepare_retained_text(ui_shaper, kFindToggleLabels[i]);
        }
        for (size_t i = 0; i < kFindButtonLabels.size(); ++i) {
            find_button_layouts_[i] = prepare_retained_text(ui_shaper, kFindButtonLabels[i]);
        }
        for (size_t i = 0; i < kSourceLines.size(); ++i) {
            highlighted_lines_[i] = highlight_line(body_shaper, kSourceLines[i]);
        }
        grapheme_shaper* gutter_shaper = grapheme_shaper::instance(gutter_font_);
        line_number_layouts_.reserve(kDocumentLineCount);
        for (size_t i = 0; i < kDocumentLineCount; ++i) {
            line_number_layouts_.push_back(
                prepare_retained_text(gutter_shaper, std::to_string(i + 1)));
        }
        for (size_t i = 0; i < kSidebarLines.size(); ++i) {
            px_font_t* font = i == 0 ? sidebar_title_font_ : sidebar_font_;
            sidebar_layouts_[i] = prepare_retained_text(font, kSidebarLines[i].text);
        }
    }

    bool handle_event(const px_event_t* event) override {
        switch (event->type) {
        case PX_EVENT_KEY:
            if (event->pressed && event->key == PX_KEY_ESCAPE) {
                if (find_panel_visible_) {
                    find_panel_visible_ = false;
                    window_->mark_dirty();
                } else {
                    window_->close();
                }
                return true;
            }
            if (event->pressed && event->key == static_cast<px_key>('0') &&
                (event->modifiers & ~PX_MOD_CAPS_LOCK) == PX_MOD_SUPER) {
                sidebar_visible_ = !sidebar_visible_;
                window_->mark_dirty();
                return true;
            }
            if (event->pressed && event->key == static_cast<px_key>('f') &&
                (event->modifiers & ~PX_MOD_CAPS_LOCK) == PX_MOD_SUPER) {
                find_panel_visible_ = true;
                window_->mark_dirty();
                return true;
            }
            if (event->pressed && event->key == PX_KEY_BACKSPACE && find_panel_visible_ &&
                !find_query_.empty()) {
                // Drop one UTF-8 codepoint: continuation bytes first, then the lead byte.
                while (!find_query_.empty() &&
                       (static_cast<unsigned char>(find_query_.back()) & 0xC0) == 0x80) {
                    find_query_.pop_back();
                }
                find_query_.pop_back();
                set_find_query(find_query_);
                return true;
            }
            break;
        case PX_EVENT_CHARACTER:
            if (find_panel_visible_ && event->text[0] != '\0' &&
                (event->modifiers & (PX_MOD_SUPER | PX_MOD_CONTROL)) == 0) {
                set_find_query(find_query_ + event->text);
                return true;
            }
            break;
        case PX_EVENT_SCROLL: {
            if (kScrollTrace) {
                std::println(stderr, "scroll ts={:.6f} dt={:.3f}ms lag={:.3f}ms delta={:.3f}",
                             event->timestamp, (event->timestamp - last_scroll_time_) * 1000.0,
                             (px_now() - event->timestamp) * 1000.0, -event->scroll_delta.y);
                last_scroll_time_ = event->timestamp;
            }
            if (event->precise_scroll) {
                if (scroll_.scroll(-event->scroll_delta.y, event->timestamp,
                                   maximum_scroll_offset())) {
                    window_->mark_dirty();
                }
            } else {
                jump_scroll_to(scroll_.offset() - event->scroll_delta.y);
            }
            window_->set_animating(scroll_.animating());
            return true;
        }
        case PX_EVENT_MOUSE_BUTTON:
            if (event->button != PX_MOUSE_LEFT) {
                break;
            }
            if (!event->pressed) {
                if (dragging_scrollbar_) {
                    dragging_scrollbar_ = false;
                    window_->mark_dirty();
                    return true;
                }
                break;
            }
            if (find_panel_visible_ && click_find_panel(event->pos)) {
                return true;
            }
            if (point_in_rect(event->pos, scrollbar_track())) {
                const rect thumb = scrollbar_thumb();
                if (point_in_rect(event->pos, thumb)) {
                    dragging_scrollbar_ = true;
                    scrollbar_drag_offset_ = event->pos.y - thumb.y;
                    window_->mark_dirty();
                } else {
                    scrollbar_drag_offset_ = thumb.h * 0.5;
                    scroll_thumb_to(event->pos.y);
                }
                return true;
            }
            if (kScrollTrace) {
                // A click anywhere else marks "something looked wrong just now" in the log.
                std::println(stderr, "mark t={:.6f}", event->timestamp);
            }
            break;
        case PX_EVENT_MOUSE_MOTION:
            if (dragging_scrollbar_) {
                scroll_thumb_to(event->pos.y);
                return true;
            }
            break;
        case PX_EVENT_CAPTURE_LOST:
            if (dragging_scrollbar_) {
                dragging_scrollbar_ = false;
                window_->mark_dirty();
                return true;
            }
            break;
        default:
            break;
        }
        return false;
    }

    void animation_tick(double now) override {
        // `now` is the frame's display time on macOS and the tick time elsewhere; the scroll
        // samples against the tick time so it means the same thing on every platform.
        const double tick_time = px_now();
        if (scroll_.tick(tick_time, maximum_scroll_offset())) {
            window_->mark_dirty();
        }
        if (kScrollTrace && scroll_.animating()) {
            std::println(stderr, "tick target={:.6f} now={:.6f} offset={:.3f}", now, tick_time,
                         scroll_.offset());
        }
        window_->set_animating(scroll_.animating());
    }

    void frame_presented(uint64_t frame_id, double presented_time) {
        const auto found = painted_offsets_.find(frame_id);
        if (found == painted_offsets_.end()) {
            return;
        }
        const double offset = found->second;
        painted_offsets_.erase(found);
        std::println(
            stderr, "present t={:.6f} interval={:.3f}ms frame={} offset={:.3f} step={:.3f}",
            presented_time,
            previous_presented_time_ == 0.0 ? 0.0
                                            : (presented_time - previous_presented_time_) * 1000.0,
            frame_id, offset, offset - previous_presented_offset_);
        previous_presented_time_ = presented_time;
        previous_presented_offset_ = offset;
    }

    void draw(px_render_context* context,
              rect bounds,
              const rect* dirty,
              int dirty_count) override {
        if (kScrollTrace) {
            if (const uint64_t frame_id = px_current_frame_id(window_->px_window())) {
                painted_offsets_.insert_or_assign(frame_id, scroll_.offset());
            }
        }
        const double sidebar_width = sidebar_visible_ ? kSidebarWidth : 0.0;
        context->begin_rect_batch();
        context->draw_rect(bounds, fcolor{1.0f, 1.0f, 1.0f, 1.0f});
        if (sidebar_visible_) {
            context->draw_rect(rect{0.0, 0.0, kSidebarWidth, bounds.h},
                               fcolor{0.955f, 0.958f, 0.963f, 1.0f});
            context->draw_rect(rect{0.0, sidebar_row_top(kSelectedSidebarLine), kSidebarWidth,
                                    sidebar_row_height()},
                               fcolor{0.86f, 0.91f, 0.98f, 1.0f});
            context->draw_rect(rect{kSidebarWidth - 1.0, 0.0, 1.0, bounds.h},
                               fcolor{0.82f, 0.83f, 0.85f, 1.0f});
        }
        context->draw_rect(rect{sidebar_width, 0.0, kGutterWidth, bounds.h},
                           fcolor{0.985f, 0.985f, 0.985f, 1.0f});
        context->draw_rect(rect{sidebar_width + kGutterWidth - 1.0, 0.0, 1.0, bounds.h},
                           fcolor{0.90f, 0.90f, 0.90f, 1.0f});
        context->end_rect_batch();

        if (sidebar_visible_ && sidebar_title_font_ && sidebar_font_) {
            context->begin_text_batch();
            for (size_t i = 0; i < sidebar_layouts_.size(); ++i) {
                const fcolor color =
                    i == 0 ? fcolor{0.45f, 0.47f, 0.51f, 1.0f} : fcolor{0.20f, 0.22f, 0.25f, 1.0f};
                px_font_t* font = i == 0 ? sidebar_title_font_ : sidebar_font_;
                const size_t indent_level = kSidebarLines[i].indent_level;
                const double indent =
                    indent_level == 0 ? 0.0
                                      : kSidebarIndentOffset + indent_level * kSidebarIndentWidth;
                const double x = kSidebarLeftPadding + indent;
                draw_retained_text(context, font, vec2{x, sidebar_text_baseline(i)}, color,
                                   &sidebar_layouts_[i]);
            }
            context->end_text_batch();
        }

        context->push_state(false);
        context->restrict_clip_rect(
            rect{sidebar_width, 0.0, bounds.w - sidebar_width, content_bottom()});
        const double scroll_offset = scroll_.offset();
        const int first_line = std::max(
            0, static_cast<int>(std::floor((scroll_offset - kTextTop) / line_height_)) - 1);
        const int last_line = std::min(
            static_cast<int>(kDocumentLineCount),
            static_cast<int>(std::ceil((scroll_offset + bounds.h - kTextTop) / line_height_)) + 1);
        context->begin_text_batch();
        for (int line = first_line; line < last_line; ++line) {
            const size_t i = static_cast<size_t>(line);
            const double y = kTextTop + i * line_height_ - scroll_offset;
            const double number_x = sidebar_width + kGutterWidth - kGutterRightPadding -
                                    line_number_layouts_[i].advance;
            draw_layout(context, gutter_font_, vec2{number_x, y},
                        fcolor{0.58f, 0.59f, 0.62f, 1.0f}, &line_number_layouts_[i]);
            draw_highlighted_line(context, body_font_,
                                  vec2{sidebar_width + kGutterWidth + kMargin, y},
                                  &highlighted_lines_[i % kSourceLines.size()]);
        }
        context->end_text_batch();
        context->pop_state();

        const rect track = scrollbar_track();
        const rect thumb = scrollbar_thumb();
        context->begin_rect_batch();
        context->draw_rect(track, fcolor{0.965f, 0.965f, 0.965f, 1.0f});
        context->draw_rect(thumb, dragging_scrollbar_ ? fcolor{0.55f, 0.56f, 0.58f, 1.0f}
                                                      : fcolor{0.70f, 0.71f, 0.73f, 1.0f});
        context->end_rect_batch();

        if (find_panel_visible_) {
            draw_find_panel(context);
        }
    }

private:
    struct FindPanelLayout {
        rect panel;
        rect close;
        std::array<rect, kFindToggleLabels.size()> toggles;
        rect input;
        std::array<rect, kFindButtonLabels.size()> buttons;
    };

    // Where the document ends: the top of the find panel when it is showing.
    double content_bottom() const {
        return window_->size().y - (find_panel_visible_ ? kFindPanelHeight : 0.0);
    }

    FindPanelLayout find_panel_layout() const {
        const vec2 size = window_->size();
        FindPanelLayout layout;
        layout.panel = rect{0.0, size.y - kFindPanelHeight, size.x, kFindPanelHeight};
        const double y = layout.panel.y + (kFindPanelHeight - kFindItemHeight) * 0.5;

        double x = kFindPanelPadding;
        layout.close = rect{x, y, kFindItemHeight, kFindItemHeight};
        x += kFindItemHeight + kFindItemGap;
        for (rect& toggle : layout.toggles) {
            toggle = rect{x, y, kFindToggleWidth, kFindItemHeight};
            x += kFindToggleWidth + kFindItemGap;
        }

        // The buttons hug the right edge but read left to right.
        double right = size.x - kFindPanelPadding;
        for (size_t i = layout.buttons.size(); i-- > 0;) {
            const double w = find_button_layouts_[i].advance + 2.0 * kFindButtonPadding;
            layout.buttons[i] = rect{right - w, y, w, kFindItemHeight};
            right -= w + kFindItemGap;
        }

        layout.input = rect{x, y, std::max(0.0, right - x), kFindItemHeight};
        return layout;
    }

    bool click_find_panel(vec2 pos) {
        const FindPanelLayout layout = find_panel_layout();
        if (!point_in_rect(pos, layout.panel)) {
            return false;
        }
        if (point_in_rect(pos, layout.close)) {
            find_panel_visible_ = false;
        }
        for (size_t i = 0; i < layout.toggles.size(); ++i) {
            if (point_in_rect(pos, layout.toggles[i])) {
                find_toggle_on_[i] = !find_toggle_on_[i];
            }
        }
        // The action buttons are visual only; a click on them just lands in the panel.
        window_->mark_dirty();
        return true;
    }

    void set_find_query(std::string query) {
        find_query_ = std::move(query);
        find_match_count_ = 0;
        if (find_query_.empty()) {
            find_query_layout_ = PreparedText{};
            find_count_layout_ = PreparedText{};
        } else {
            find_query_layout_ =
                prepare_retained_text(grapheme_shaper::instance(body_font_), find_query_);
            // The document repeats kSourceLines, so count each source line once and weight it by
            // how many document lines it stands for.
            for (size_t i = 0; i < kSourceLines.size(); ++i) {
                size_t per_line = 0;
                for (size_t at = kSourceLines[i].find(find_query_); at != std::string_view::npos;
                     at = kSourceLines[i].find(find_query_, at + find_query_.size())) {
                    ++per_line;
                }
                const size_t repeats = kDocumentLineCount / kSourceLines.size() +
                                       (i < kDocumentLineCount % kSourceLines.size() ? 1 : 0);
                find_match_count_ += per_line * repeats;
            }
            find_count_layout_ =
                prepare_retained_text(grapheme_shaper::instance(sidebar_font_),
                                      std::to_string(find_match_count_) +
                                          (find_match_count_ == 1 ? " match" : " matches"));
        }
        window_->mark_dirty();
    }

    static void draw_box(px_render_context* context, rect box, fcolor fill, fcolor border) {
        context->draw_rect(box, border);
        context->draw_rect(rect{box.x + 1.0, box.y + 1.0, box.w - 2.0, box.h - 2.0}, fill);
    }

    // Centers a prepared label in a box.
    static void draw_centered(px_render_context* context,
                              px_font_t* font,
                              const px_font_metrics& metrics,
                              rect box,
                              fcolor color,
                              PreparedText* text) {
        const double x = box.x + (box.w - text->advance) * 0.5;
        const double baseline = box.y + (box.h - metrics.line_height) * 0.5 + metrics.ascent;
        draw_layout(context, font, vec2{x, baseline}, color, text);
    }

    void draw_find_panel(px_render_context* context) {
        const FindPanelLayout layout = find_panel_layout();

        context->begin_rect_batch();
        context->draw_rect(layout.panel, kFindPanelBackground);
        context->draw_rect(rect{layout.panel.x, layout.panel.y, layout.panel.w, 1.0},
                           kFindPanelBorder);
        for (size_t i = 0; i < layout.toggles.size(); ++i) {
            draw_box(context, layout.toggles[i],
                     find_toggle_on_[i] ? kFindToggleOnBackground : kFindButtonBackground,
                     kFindButtonBorder);
        }
        draw_box(context, layout.input, kFindInputBackground, kFindInputBorder);
        for (const rect& button : layout.buttons) {
            draw_box(context, button, kFindButtonBackground, kFindButtonBorder);
        }
        const double caret_x = layout.input.x + kFindTextInset + find_query_layout_.advance;
        context->draw_rect(rect{caret_x, layout.input.y + 5.0, 1.0, kFindItemHeight - 10.0},
                           kFindLabelColor);
        context->end_rect_batch();

        context->begin_text_batch();
        draw_centered(context, sidebar_font_, sidebar_metrics_, layout.close, kFindMutedColor,
                      &find_close_layout_);
        for (size_t i = 0; i < layout.toggles.size(); ++i) {
            draw_centered(context, sidebar_font_, sidebar_metrics_, layout.toggles[i],
                          kFindLabelColor, &find_toggle_layouts_[i]);
        }
        for (size_t i = 0; i < layout.buttons.size(); ++i) {
            draw_centered(context, sidebar_font_, sidebar_metrics_, layout.buttons[i],
                          kFindLabelColor, &find_button_layouts_[i]);
        }
        if (!find_query_.empty()) {
            const double baseline = layout.input.y +
                                    (kFindItemHeight - body_metrics_.line_height) * 0.5 +
                                    body_metrics_.ascent;
            draw_layout(context, body_font_, vec2{layout.input.x + kFindTextInset, baseline},
                        kFindLabelColor, &find_query_layout_);
            const double count_baseline = layout.input.y +
                                          (kFindItemHeight - sidebar_metrics_.line_height) * 0.5 +
                                          sidebar_metrics_.ascent;
            draw_layout(context, sidebar_font_,
                        vec2{layout.input.right() - kFindTextInset - find_count_layout_.advance,
                             count_baseline},
                        kFindMutedColor, &find_count_layout_);
        }
        context->end_text_batch();
    }

    double sidebar_row_height() const {
        return kSidebarRowTopPadding + sidebar_title_metrics_.line_height +
               kSidebarRowBottomPadding;
    }

    double sidebar_row_top(size_t index) const {
        return kSidebarTopPadding + index * sidebar_row_height();
    }

    double sidebar_text_baseline(size_t index) const {
        const px_font_metrics& metrics = index == 0 ? sidebar_title_metrics_ : sidebar_metrics_;
        return sidebar_row_top(index) + kSidebarRowTopPadding +
               (sidebar_title_metrics_.line_height - metrics.line_height) * 0.5 + metrics.ascent;
    }

    double document_height() const { return kTextTop + kDocumentLineCount * line_height_; }

    double maximum_scroll_offset() const {
        return std::max(0.0, document_height() - content_bottom());
    }

    rect scrollbar_track() const {
        const vec2 size = window_->size();
        return rect{size.x - kScrollbarMargin - kScrollbarWidth, kScrollbarTop, kScrollbarWidth,
                    std::max(0.0, content_bottom() - kScrollbarTop - kScrollbarMargin)};
    }

    rect scrollbar_thumb() const {
        const rect track = scrollbar_track();
        const double viewport_height = content_bottom();
        const double thumb_height = std::min(
            track.h, std::max(kMinimumThumbHeight, track.h * viewport_height / document_height()));
        const double travel = track.h - thumb_height;
        const double maximum_offset = maximum_scroll_offset();
        const double progress = maximum_offset > 0.0 ? scroll_.offset() / maximum_offset : 0.0;
        return rect{track.x + 2.0, track.y + travel * progress, track.w - 4.0, thumb_height};
    }

    static bool point_in_rect(vec2 point, rect bounds) {
        return point.x >= bounds.x && point.x < bounds.right() && point.y >= bounds.y &&
               point.y < bounds.bottom();
    }

    void jump_scroll_to(double offset) {
        scroll_.jump_to(offset, maximum_scroll_offset());
        window_->set_animating(false);
        window_->mark_dirty();
    }

    void scroll_thumb_to(double pointer_y) {
        const rect track = scrollbar_track();
        const rect thumb = scrollbar_thumb();
        const double travel = track.h - thumb.h;
        if (travel <= 0.0) {
            return;
        }
        const double thumb_y =
            std::clamp(pointer_y - scrollbar_drag_offset_, track.y, track.bottom() - thumb.h);
        jump_scroll_to((thumb_y - track.y) / travel * maximum_scroll_offset());
    }

    window* window_ = nullptr;
    px_font_t* body_font_ = nullptr;
    px_font_t* sidebar_title_font_ = nullptr;
    px_font_t* sidebar_font_ = nullptr;
    px_font_t* gutter_font_ = nullptr;
    double line_height_ = 0.0;
    px_font_metrics body_metrics_;
    px_font_metrics sidebar_title_metrics_;
    px_font_metrics sidebar_metrics_;
    smooth_scroll scroll_;
    double last_scroll_time_ = 0.0;  // trace only
    std::unordered_map<uint64_t, double> painted_offsets_;
    double previous_presented_time_ = 0.0;
    double previous_presented_offset_ = 0.0;
    bool sidebar_visible_ = true;
    bool find_panel_visible_ = true;
    std::string find_query_;
    size_t find_match_count_ = 0;
    PreparedText find_query_layout_;
    PreparedText find_count_layout_;
    PreparedText find_close_layout_;
    std::array<PreparedText, kFindToggleLabels.size()> find_toggle_layouts_;
    std::array<bool, kFindToggleLabels.size()> find_toggle_on_{};
    std::array<PreparedText, kFindButtonLabels.size()> find_button_layouts_;
    bool dragging_scrollbar_ = false;
    double scrollbar_drag_offset_ = 0.0;
    std::array<HighlightedLine, kSourceLines.size()> highlighted_lines_;
    std::vector<PreparedText> line_number_layouts_;
    std::array<PreparedText, kSidebarLines.size()> sidebar_layouts_;
};

}  // namespace

int main(int argc, char** argv) {
    px_init("editor", "com.example.editor", argc, argv, 0);

    window_impl window(1000.0, 700.0, "editor", fcolor{1.0f, 1.0f, 1.0f, 1.0f});
    window_basic_aspect basic(&window);
    window.add_window_aspect(&basic);

    px_font_t* body_font = px_create_font("Source Code Pro", kMainFontSize);
    px_font_t* sidebar_title_font = px_create_font("system", kSidebarTitleFontSize, PX_FONT_BOLD);
    px_font_t* sidebar_font = px_create_font("system", kSidebarFontSize);
    px_font_t* gutter_font = px_create_font("Source Code Pro", kMainFontSize);
    EditorControl root(&window, body_font, sidebar_title_font, sidebar_font, gutter_font);
    window.set_root_control(&root);
    if (kScrollTrace) {
        px_set_frame_presented_callback(window.px_window(),
                                        [&root](uint64_t frame_id, double presented_time) {
                                            root.frame_presented(frame_id, presented_time);
                                        });
    }

    window.set_maximized(true);
    window.show();
    px_run_event_loop();
    return 0;
}
