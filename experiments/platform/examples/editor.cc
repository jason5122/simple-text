#include "experiments/platform/px/px.h"
#include "experiments/platform/ui/retained_text.h"
#include "experiments/platform/ui/window.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <print>
#include <string>
#include <string_view>
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
          sidebar_title_metrics_(px_font_get_metrics(sidebar_title_font)),
          sidebar_metrics_(px_font_get_metrics(sidebar_font)) {
        std::println("sidebar title line height: {}", sidebar_title_metrics_.line_height);
        grapheme_shaper* body_shaper = grapheme_shaper::instance(body_font_);
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
                window_->close();
                return true;
            }
            if (event->pressed && event->key == static_cast<px_key>('0') &&
                (event->modifiers & ~PX_MOD_CAPS_LOCK) == PX_MOD_SUPER) {
                sidebar_visible_ = !sidebar_visible_;
                window_->mark_dirty();
                return true;
            }
            break;
        case PX_EVENT_SCROLL: {
            set_scroll_offset(scroll_offset_ - event->scroll_delta.y);
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

    void draw(px_render_context* context,
              rect bounds,
              const rect* dirty,
              int dirty_count) override {
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
        }

        context->push_state(false);
        context->restrict_clip_rect(rect{sidebar_width, 0.0, bounds.w - sidebar_width, bounds.h});
        const int first_line = std::max(
            0, static_cast<int>(std::floor((scroll_offset_ - kTextTop) / line_height_)) - 1);
        const int last_line = std::min(
            static_cast<int>(kDocumentLineCount),
            static_cast<int>(std::ceil((scroll_offset_ + bounds.h - kTextTop) / line_height_)) +
                1);
        for (int line = first_line; line < last_line; ++line) {
            const size_t i = static_cast<size_t>(line);
            const double y = kTextTop + i * line_height_ - scroll_offset_;
            const double number_x = sidebar_width + kGutterWidth - kGutterRightPadding -
                                    line_number_layouts_[i].advance;
            draw_layout(context, gutter_font_, vec2{number_x, y},
                        fcolor{0.58f, 0.59f, 0.62f, 1.0f}, &line_number_layouts_[i]);
            draw_highlighted_line(context, body_font_,
                                  vec2{sidebar_width + kGutterWidth + kMargin, y},
                                  &highlighted_lines_[i % kSourceLines.size()]);
        }
        context->pop_state();

        const rect track = scrollbar_track();
        const rect thumb = scrollbar_thumb();
        context->begin_rect_batch();
        context->draw_rect(track, fcolor{0.965f, 0.965f, 0.965f, 1.0f});
        context->draw_rect(thumb, dragging_scrollbar_ ? fcolor{0.55f, 0.56f, 0.58f, 1.0f}
                                                      : fcolor{0.70f, 0.71f, 0.73f, 1.0f});
        context->end_rect_batch();
    }

private:
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
        return std::max(0.0, document_height() - window_->size().y);
    }

    rect scrollbar_track() const {
        const vec2 size = window_->size();
        return rect{size.x - kScrollbarMargin - kScrollbarWidth, kScrollbarTop, kScrollbarWidth,
                    std::max(0.0, size.y - kScrollbarTop - kScrollbarMargin)};
    }

    rect scrollbar_thumb() const {
        const rect track = scrollbar_track();
        const double viewport_height = window_->size().y;
        const double thumb_height = std::min(
            track.h, std::max(kMinimumThumbHeight, track.h * viewport_height / document_height()));
        const double travel = track.h - thumb_height;
        const double maximum_offset = maximum_scroll_offset();
        const double progress = maximum_offset > 0.0 ? scroll_offset_ / maximum_offset : 0.0;
        return rect{track.x + 2.0, track.y + travel * progress, track.w - 4.0, thumb_height};
    }

    static bool point_in_rect(vec2 point, rect bounds) {
        return point.x >= bounds.x && point.x < bounds.right() && point.y >= bounds.y &&
               point.y < bounds.bottom();
    }

    void set_scroll_offset(double offset) {
        const double next_offset = std::clamp(offset, 0.0, maximum_scroll_offset());
        if (next_offset != scroll_offset_) {
            scroll_offset_ = next_offset;
            window_->mark_dirty();
        }
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
        set_scroll_offset((thumb_y - track.y) / travel * maximum_scroll_offset());
    }

    window* window_ = nullptr;
    px_font_t* body_font_ = nullptr;
    px_font_t* sidebar_title_font_ = nullptr;
    px_font_t* sidebar_font_ = nullptr;
    px_font_t* gutter_font_ = nullptr;
    double line_height_ = 0.0;
    px_font_metrics sidebar_title_metrics_;
    px_font_metrics sidebar_metrics_;
    double scroll_offset_ = 0.0;
    bool sidebar_visible_ = true;
    bool dragging_scrollbar_ = false;
    double scrollbar_drag_offset_ = 0.0;
    std::array<HighlightedLine, kSourceLines.size()> highlighted_lines_;
    std::vector<PreparedText> line_number_layouts_;
    std::array<PreparedText, kSidebarLines.size()> sidebar_layouts_;
};

}  // namespace

int main(int argc, char** argv) {
#if defined(__APPLE__)
    setenv("PX_NO_DISPLAY_LINK", "1", 1);
#endif
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

    window.set_maximized(true);
    window.show();
    px_run_event_loop();
    return 0;
}
