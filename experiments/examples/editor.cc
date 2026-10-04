#include "px/px.h"
#include "ui/retained_text.h"
#include "ui/smooth_scroll.h"
#include "ui/window.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <format>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

constexpr const char* kBodyFontFamily = "Source Code Pro";
constexpr float kMainFontSize = 16.0f;
constexpr float kMinimumFontSize = 2.0f;
constexpr float kMaximumFontSize = 128.0f;
constexpr float kSidebarTitleFontSize = 13.0f;
constexpr float kSidebarFontSize = 12.0f;
constexpr float kStatusBarFontSize = 11.0f;

constexpr double kSidebarWidth = 250.0;
constexpr double kSidebarTopPadding = 10.0;
constexpr double kSidebarLeftPadding = 16.0;
constexpr double kSidebarIndentWidth = 12.0;
constexpr double kSidebarIndentOffset = 5.0;
constexpr double kSidebarRowTopPadding = 3.0;
constexpr double kSidebarRowBottomPadding = 3.0;
constexpr size_t kSelectedSidebarLine = 1;

// The gutter is sized the way Sublime sizes its own (calc_gutter_width in the 4212 binary,
// checked against captures at 8-48px): the line numbers are right-aligned in a box
// ceil(14 + digits * advance('0')) wide, with at least two digits, preceded by the 4px mini-diff
// strip and followed by 3px, another 4px for mini-diff, the 4px "margin" setting and 13px of
// fold buttons. The advance comes from the shaper, which snaps monospace advances to whole
// pixels at 16px and below just as Sublime does, so the width matches at every size.
constexpr double kGutterLeftPadding = 14.0;
constexpr double kGutterNumberPadding = 3.0;
constexpr double kGutterMiniDiffWidth = 4.0;
constexpr double kGutterMargin = 4.0;
constexpr double kGutterFoldButtonWidth = 13.0;
constexpr int kMinimumLineNumberDigits = 2;
constexpr double kMargin = 1.0;
constexpr double kTextTop = 50.0;

constexpr double kScrollbarTop = 38.0;
constexpr double kScrollbarWidth = 7.0;
constexpr double kScrollbarMargin = 4.0;

constexpr double kMinimumThumbHeight = 36.0;
// A press this close to the thumb takes hold of it where it is instead of jumping it under the
// pointer.
constexpr double kThumbGrabMargin = 16.0;
constexpr size_t kDocumentLineCount = 500;

constexpr int digit_count(size_t value) {
    int digits = 1;
    for (; value >= 10; value /= 10) {
        ++digits;
    }
    return digits;
}

constexpr int kLineNumberDigits =
    std::max(kMinimumLineNumberDigits, digit_count(kDocumentLineCount));

// The find panel along the bottom, after Sublime's: a close mark, four option toggles, the query
// field with its match count, and the three action buttons hugging the right edge. It starts
// hidden; Cmd+F shows it, Escape hides it. The document area ends where the panel begins.
constexpr double kFindPanelHeight = 40.0;
constexpr double kFindPanelPadding = 8.0;
constexpr double kFindItemHeight = 24.0;
constexpr double kFindItemGap = 6.0;
constexpr double kFindToggleWidth = 28.0;
constexpr double kFindButtonPadding = 12.0;
constexpr double kFindTextInset = 7.0;

// The tab strip along the top of the document area, to the right of the sidebar. Empty for now.
constexpr double kTabStripHeight = 32.0;

// The status bar along the very bottom, below the find panel: a caret position on the left.
constexpr double kStatusBarHeight = 22.0;
constexpr double kStatusBarPadding = 16.0;

// Where scroll and drag frames come from. Where the OS smooths frames painted from the events
// (a build linked against the macOS 26 SDK; PX_OS_SMOOTHS_EVENT_FRAMES in px/px.h), every scroll
// event adds its delta and every mouse event moves the thumb, each painting its own frame in the
// event's own turn, with nothing sampled and no display link: Sublime Text's model. Elsewhere the
// display link paints, sampling the input's trajectory a little behind each tick
// (ui/smooth_scroll). Replace the initializer to try the other path.
constexpr bool kPaintOnEvents = PX_OS_SMOOTHS_EVENT_FRAMES;

// Colours. The light values are Sublime's; the dark palette sits beside it, field for field, so
// either can be edited in place. Cmd-3 switches between them, as between Sublime's Default and
// Default Dark themes.
struct Palette {
    fcolor buffer_background;
    fcolor sidebar_background;
    fcolor sidebar_selection;
    fcolor sidebar_title;
    fcolor sidebar_text;
    fcolor tab_strip_background;
    fcolor find_panel_background;
    fcolor find_input_background;
    fcolor find_button_background;
    fcolor find_toggle_on_background;
    fcolor find_label;
    fcolor find_muted;
    fcolor status_bar_background;
    fcolor status_bar_text;
    fcolor line_number;
    // Rows whose text needs a fallback font (the emoji line) get this band, sized from the body
    // font's metrics exactly as Sublime sizes a line, so the glyphs can be checked against the
    // row.
    fcolor line_band;
    // Cmd-2 paints the gutter in this so its extent shows against the sidebar and buffer.
    fcolor gutter_highlight;
    fcolor scrollbar_track;
    fcolor scrollbar_thumb;
    fcolor scrollbar_thumb_dragging;
    // Syntax.
    fcolor plain_text;
    fcolor keyword_text;
    fcolor type_text;
    fcolor function_text;
    fcolor string_text;
    fcolor number_text;
    fcolor comment_text;
};

constexpr Palette kLightPalette{
    .buffer_background = fcolor{1.0f, 1.0f, 1.0f, 1.0f},
    // Sublime's sidebar background, (235, 237, 239); its "FOLDERS" heading, (128, 128, 128), and
    // the rows beneath it, (51, 51, 51).
    .sidebar_background = fcolor{235.0f / 255.0f, 237.0f / 255.0f, 239.0f / 255.0f, 1.0f},
    .sidebar_selection = fcolor{224.0f / 255.0f, 227.0f / 255.0f, 230.0f / 255.0f, 1.0f},
    .sidebar_title = fcolor{128.0f / 255.0f, 128.0f / 255.0f, 128.0f / 255.0f, 1.0f},
    .sidebar_text = fcolor{51.0f / 255.0f, 51.0f / 255.0f, 51.0f / 255.0f, 1.0f},
    // Sublime's tab strip, (189, 190, 190), and its find panel and status bar, (199, 203, 209),
    // with the status bar's text at (64, 64, 64).
    .tab_strip_background = fcolor{189.0f / 255.0f, 190.0f / 255.0f, 190.0f / 255.0f, 1.0f},
    .find_panel_background = fcolor{199.0f / 255.0f, 203.0f / 255.0f, 209.0f / 255.0f, 1.0f},
    .find_input_background = fcolor{1.0f, 1.0f, 1.0f, 1.0f},
    .find_button_background = fcolor{0.985f, 0.985f, 0.99f, 1.0f},
    .find_toggle_on_background = fcolor{0.80f, 0.86f, 0.96f, 1.0f},
    .find_label = fcolor{0.22f, 0.23f, 0.26f, 1.0f},
    .find_muted = fcolor{0.50f, 0.52f, 0.56f, 1.0f},
    .status_bar_background = fcolor{199.0f / 255.0f, 203.0f / 255.0f, 209.0f / 255.0f, 1.0f},
    .status_bar_text = fcolor{64.0f / 255.0f, 64.0f / 255.0f, 64.0f / 255.0f, 1.0f},
    .line_number = fcolor{152.0f / 255.0f, 152.0f / 255.0f, 152.0f / 255.0f, 1.0f},
    .line_band = fcolor{0.86f, 0.91f, 0.98f, 0.45f},
    .gutter_highlight = fcolor{0.85f, 0.85f, 0.85f, 1.0f},
    .scrollbar_track = fcolor{235.0f / 255.0f, 236.0f / 255.0f, 236.0f / 255.0f, 1.0f},
    .scrollbar_thumb = fcolor{203.0f / 255.0f, 204.0f / 255.0f, 204.0f / 255.0f, 1.0f},
    .scrollbar_thumb_dragging = fcolor{140.0f / 255.0f, 141.0f / 255.0f, 141.0f / 255.0f, 1.0f},
    .plain_text = fcolor{0.23f, 0.24f, 0.27f, 1.0f},
    .keyword_text = fcolor{0.50f, 0.22f, 0.68f, 1.0f},
    .type_text = fcolor{0.10f, 0.40f, 0.67f, 1.0f},
    .function_text = fcolor{0.13f, 0.46f, 0.55f, 1.0f},
    .string_text = fcolor{0.72f, 0.28f, 0.19f, 1.0f},
    .number_text = fcolor{0.76f, 0.38f, 0.08f, 1.0f},
    .comment_text = fcolor{0.36f, 0.48f, 0.37f, 1.0f},
};

constexpr Palette kDarkPalette{
    .buffer_background = fcolor{48.0f / 255.0f, 56.0f / 255.0f, 65.0f / 255.0f, 1.0f},
    .sidebar_background = fcolor{34.0f / 255.0f, 38.0f / 255.0f, 42.0f / 255.0f, 1.0f},
    .sidebar_selection = fcolor{41.0f / 255.0f, 45.0f / 255.0f, 50.0f / 255.0f, 1.0f},
    .sidebar_title = fcolor{230.0f / 255.0f, 230.0f / 255.0f, 230.0f / 255.0f, 1.0f},
    .sidebar_text = fcolor{204.0f / 255.0f, 204.0f / 255.0f, 204.0f / 255.0f, 1.0f},
    .tab_strip_background = fcolor{79.0f / 255.0f, 86.0f / 255.0f, 94.0f / 255.0f, 1.0f},
    .find_panel_background = fcolor{0.16f, 0.175f, 0.205f, 1.0f},
    .find_input_background = fcolor{0.075f, 0.082f, 0.098f, 1.0f},
    .find_button_background = fcolor{0.21f, 0.23f, 0.27f, 1.0f},
    .find_toggle_on_background = fcolor{0.25f, 0.40f, 0.60f, 1.0f},
    .find_label = fcolor{0.80f, 0.83f, 0.88f, 1.0f},
    .find_muted = fcolor{0.52f, 0.55f, 0.62f, 1.0f},
    .status_bar_background = fcolor{46.0f / 255.0f, 50.0f / 255.0f, 56.0f / 255.0f, 1.0f},
    .status_bar_text = fcolor{217.0f / 255.0f, 217.0f / 255.0f, 217.0f / 255.0f, 1.0f},
    .line_number = fcolor{132.0f / 255.0f, 139.0f / 255.0f, 149.0f / 255.0f, 1.0f},
    .line_band = fcolor{0.105f, 0.135f, 0.185f, 0.32f},
    .gutter_highlight = fcolor{0.16f, 0.175f, 0.205f, 1.0f},
    .scrollbar_track = fcolor{67.0f / 255.0f, 75.0f / 255.0f, 83.0f / 255.0f, 1.0f},
    .scrollbar_thumb = fcolor{105.0f / 255.0f, 112.0f / 255.0f, 118.0f / 255.0f, 1.0f},
    .scrollbar_thumb_dragging = fcolor{180.0f / 255.0f, 183.0f / 255.0f, 187.0f / 255.0f, 1.0f},
    .plain_text = fcolor{0.78f, 0.80f, 0.86f, 1.0f},
    .keyword_text = fcolor{0.72f, 0.52f, 0.91f, 1.0f},
    .type_text = fcolor{0.48f, 0.72f, 0.96f, 1.0f},
    .function_text = fcolor{0.42f, 0.78f, 0.82f, 1.0f},
    .string_text = fcolor{0.91f, 0.58f, 0.36f, 1.0f},
    .number_text = fcolor{0.93f, 0.68f, 0.40f, 1.0f},
    .comment_text = fcolor{0.50f, 0.75f, 0.58f, 1.0f},
};

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
    "}  // Emoji and color glyphs: 👋 🌍 ✨ 🚀",
};

using PreparedText = retained_text;

enum class Token { plain, keyword, type, function, string, number, comment };

struct HighlightedRun {
    double x_offset = 0.0;
    Token token = Token::plain;
    PreparedText text;
};

using HighlightedLine = std::vector<HighlightedRun>;

fcolor token_color(const Palette& palette, Token token) {
    switch (token) {
    case Token::keyword:
        return palette.keyword_text;
    case Token::type:
        return palette.type_text;
    case Token::function:
        return palette.function_text;
    case Token::string:
        return palette.string_text;
    case Token::number:
        return palette.number_text;
    case Token::comment:
        return palette.comment_text;
    case Token::plain:
        break;
    }
    return palette.plain_text;
}

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

constexpr bool has_non_ascii(std::string_view text) {
    for (unsigned char c : text) {
        if (c >= 0x80) {
            return true;
        }
    }
    return false;
}

bool is_identifier_start(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool is_identifier_continue(char c) { return is_identifier_start(c) || (c >= '0' && c <= '9'); }

HighlightedLine highlight_line(grapheme_shaper* shaper, std::string_view text) {
    HighlightedLine result;
    double x = 0.0;
    auto append = [&](std::string_view piece, Token token) {
        PreparedText prepared = prepare_retained_text(shaper, piece);
        result.push_back({.x_offset = x, .token = token, .text = std::move(prepared)});
        x += result.back().text.advance;
    };

    for (size_t start = 0; start < text.size();) {
        size_t end = start + 1;
        Token token = Token::plain;
        const char c = text[start];

        if (c == '/' && end < text.size() && text[end] == '/') {
            end = text.size();
            token = Token::comment;
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
            token = Token::string;
        } else if (c >= '0' && c <= '9') {
            while (end < text.size() && ((text[end] >= '0' && text[end] <= '9') ||
                                         text[end] == '.' || text[end] == 'f')) {
                ++end;
            }
            token = Token::number;
        } else if (is_identifier_start(c)) {
            while (end < text.size() && is_identifier_continue(text[end])) {
                ++end;
            }
            const std::string_view word = text.substr(start, end - start);
            size_t next = end;
            while (next < text.size() && text[next] == ' ') {
                ++next;
            }
            if (contains_token(kKeywords, word)) {
                token = Token::keyword;
            } else if (contains_token(kTypes, word)) {
                token = Token::type;
            } else if (next < text.size() && text[next] == '(') {
                token = Token::function;
            }
        }

        append(text.substr(start, end - start), token);
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
                           HighlightedLine* line,
                           const Palette& palette) {
    for (HighlightedRun& run : *line) {
        draw_layout(context, font, vec2{position.x + run.x_offset, position.y},
                    token_color(palette, run.token), &run.text);
    }
}

class EditorControl final : public control {
public:
    EditorControl(window* window,
                  px_font_t* sidebar_title_font,
                  px_font_t* sidebar_font,
                  float body_font_size)
        : window_(window),
          sidebar_title_font_(sidebar_title_font),
          sidebar_font_(sidebar_font),
          sidebar_title_metrics_(px_font_get_metrics(sidebar_title_font)),
          sidebar_metrics_(px_font_get_metrics(sidebar_font)) {
        grapheme_shaper* ui_shaper = grapheme_shaper::instance(sidebar_font_);
        find_close_layout_ = prepare_retained_text(ui_shaper, "×");
        status_font_ = px_create_font("system", kStatusBarFontSize);
        status_metrics_ = px_font_get_metrics(status_font_);
        status_layout_ = prepare_retained_text(status_font_, "Line 1, Column 1");
        for (size_t i = 0; i < kFindToggleLabels.size(); ++i) {
            find_toggle_layouts_[i] = prepare_retained_text(ui_shaper, kFindToggleLabels[i]);
        }
        for (size_t i = 0; i < kFindButtonLabels.size(); ++i) {
            find_button_layouts_[i] = prepare_retained_text(ui_shaper, kFindButtonLabels[i]);
        }
        for (size_t i = 0; i < kSidebarLines.size(); ++i) {
            px_font_t* font = i == 0 ? sidebar_title_font_ : sidebar_font_;
            sidebar_layouts_[i] = prepare_retained_text(font, kSidebarLines[i].text);
        }
        set_body_font_size(body_font_size);
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
            if (event->pressed && event->key == static_cast<px_key>('2') &&
                (event->modifiers & ~PX_MOD_CAPS_LOCK) == PX_MOD_SUPER) {
                highlight_gutter_ = !highlight_gutter_;
                window_->mark_dirty();
                return true;
            }
            if (event->pressed && event->key == static_cast<px_key>('3') &&
                (event->modifiers & ~PX_MOD_CAPS_LOCK) == PX_MOD_SUPER) {
                dark_mode_ = !dark_mode_;
                window_->mark_dirty();
                return true;
            }
            // Cmd-Plus arrives as '+' with Shift held on a US layout, so Shift is ignored here.
            if (event->pressed &&
                (event->modifiers & ~(PX_MOD_CAPS_LOCK | PX_MOD_SHIFT)) == PX_MOD_SUPER) {
                if (event->key == static_cast<px_key>('=') ||
                    event->key == static_cast<px_key>('+')) {
                    adjust_body_font_size(1.0f);
                    return true;
                }
                if (event->key == static_cast<px_key>('-')) {
                    adjust_body_font_size(-1.0f);
                    return true;
                }
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
        case PX_EVENT_RESIZE:
            size_ = event->size;
            // A taller window can leave the content past the new end of the range. It rests on
            // the end, as it does when the find panel closes or the font shrinks; otherwise the
            // next scroll event would snap it there.
            if (scroll_.offset() > maximum_scroll_offset()) {
                jump_scroll_to(scroll_.offset());
            }
            break;
        case PX_EVENT_SCROLL:
            if (event->precise_scroll && !kPaintOnEvents) {
                // The display link paints; the event only feeds the trajectory.
                if (scroll_.scroll(-event->scroll_delta.y, event->timestamp,
                                   maximum_scroll_offset())) {
                    window_->mark_dirty();
                }
                window_->set_animating(scroll_.animating());
            } else {
                // The event's own frame, painted by the commit that follows it.
                jump_scroll_to(scroll_.offset() - event->scroll_delta.y);
            }
            return true;
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
                // On or near the thumb, the press takes hold of it where it is; further off, the
                // thumb jumps under the pointer first. Either way the drag is on from here.
                bool near_thumb = event->pos.y >= thumb.y - kThumbGrabMargin &&
                                  event->pos.y <= thumb.bottom() + kThumbGrabMargin;
                if (near_thumb) {
                    scrollbar_drag_offset_ = event->pos.y - thumb.y;
                } else {
                    scrollbar_drag_offset_ = thumb.h * 0.5;
                    scroll_thumb_to(event->pos.y);
                }
                dragging_scrollbar_ = true;
                window_->mark_dirty();
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

    void animation_tick(double now) override {
        // Only the display-link path gets here; with kPaintOnEvents nothing starts the link.
        // `now` is the frame's display time on macOS and the tick time elsewhere; the scroll
        // samples against the tick time so it means the same thing on every platform.
        const double tick_time = px_now();
        if (scroll_.tick(tick_time, maximum_scroll_offset())) {
            window_->mark_dirty();
        }
        window_->set_animating(scroll_.animating());
    }

    void draw(px_render_context* context,
              rect bounds,
              const rect* dirty,
              int dirty_count) override {
        size_ = vec2{bounds.w, bounds.h};
        const double sidebar_width = sidebar_visible_ ? kSidebarWidth : 0.0;
        context->begin_rect_batch();
        context->draw_rect(bounds, palette().buffer_background);
        if (sidebar_visible_) {
            context->draw_rect(rect{0.0, 0.0, kSidebarWidth, bounds.h},
                               palette().sidebar_background);
            context->draw_rect(rect{0.0, sidebar_row_top(kSelectedSidebarLine), kSidebarWidth,
                                    sidebar_row_height()},
                               palette().sidebar_selection);
        }
        // The gutter shares the buffer's background, which the fill above already painted,
        // unless Cmd-2 has asked to see it.
        if (highlight_gutter_) {
            context->draw_rect(rect{sidebar_width, 0.0, gutter_width_, bounds.h},
                               palette().gutter_highlight);
        }
        context->draw_rect(rect{sidebar_width, 0.0, bounds.w - sidebar_width, kTabStripHeight},
                           palette().tab_strip_background);
        context->end_rect_batch();

        if (sidebar_visible_ && sidebar_title_font_ && sidebar_font_) {
            context->begin_text_batch();
            for (size_t i = 0; i < sidebar_layouts_.size(); ++i) {
                const fcolor color = i == 0 ? palette().sidebar_title : palette().sidebar_text;
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
        context->restrict_clip_rect(rect{sidebar_width, kTabStripHeight, bounds.w - sidebar_width,
                                         content_bottom() - kTabStripHeight});
        const double scroll_offset = scroll_.offset();
        const int first_line = std::max(
            0, static_cast<int>(std::floor((scroll_offset - kTextTop) / line_height_)) - 1);
        const int last_line = std::min(
            static_cast<int>(kDocumentLineCount),
            static_cast<int>(std::ceil((scroll_offset + bounds.h - kTextTop) / line_height_)) + 1);
        context->begin_rect_batch();
        for (int line = first_line; line < last_line; ++line) {
            const size_t i = static_cast<size_t>(line);
            if (!has_non_ascii(kSourceLines[i % kSourceLines.size()])) {
                continue;
            }
            const double baseline = kTextTop + i * line_height_ - scroll_offset;
            context->draw_rect(rect{sidebar_width + gutter_width_, baseline - body_metrics_.ascent,
                                    bounds.w - sidebar_width - gutter_width_, line_height_},
                               palette().line_band);
        }
        context->end_rect_batch();
        context->begin_text_batch();
        for (int line = first_line; line < last_line; ++line) {
            const size_t i = static_cast<size_t>(line);
            const double y = kTextTop + i * line_height_ - scroll_offset;
            double number_x = sidebar_width + line_number_right_ - line_number_layouts_[i].advance;
            draw_layout(context, body_font_, vec2{number_x, y}, palette().line_number,
                        &line_number_layouts_[i]);
            draw_highlighted_line(context, body_font_,
                                  vec2{sidebar_width + gutter_width_ + kMargin, y},
                                  &highlighted_lines_[i % kSourceLines.size()], palette());
        }
        context->end_text_batch();
        context->pop_state();

        const rect track = scrollbar_track();
        const rect thumb = scrollbar_thumb();
        context->begin_rect_batch();
        context->draw_rect(track, palette().scrollbar_track);
        context->draw_rect(thumb, dragging_scrollbar_ ? palette().scrollbar_thumb_dragging
                                                      : palette().scrollbar_thumb);
        context->end_rect_batch();

        if (find_panel_visible_) {
            draw_find_panel(context);
        }
        draw_status_bar(context);
    }

private:
    struct FindPanelLayout {
        rect panel;
        rect close;
        std::array<rect, kFindToggleLabels.size()> toggles;
        rect input;
        std::array<rect, kFindButtonLabels.size()> buttons;
    };

    const Palette& palette() const { return dark_mode_ ? kDarkPalette : kLightPalette; }

    // Where the document ends: the top of the status bar, or of the find panel when it is showing.
    double content_bottom() const {
        return size_.y - kStatusBarHeight - (find_panel_visible_ ? kFindPanelHeight : 0.0);
    }

    FindPanelLayout find_panel_layout() const {
        const vec2 size = size_;
        FindPanelLayout layout;
        layout.panel =
            rect{0.0, size.y - kStatusBarHeight - kFindPanelHeight, size.x, kFindPanelHeight};
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

    // (Re)builds everything shaped in the body font, which the gutter shares.
    void set_body_font_size(float size) {
        size = std::clamp(size, kMinimumFontSize, kMaximumFontSize);
        if (body_font_ && size == body_font_size_) {
            return;
        }
        body_font_size_ = size;
        body_font_ = px_create_font(kBodyFontFamily, size);
        body_metrics_ = px_font_get_metrics(body_font_);
        line_height_ = body_metrics_.line_height;
        grapheme_shaper* body_shaper = grapheme_shaper::instance(body_font_);
        double zero_advance = prepare_retained_text(body_shaper, "0").advance;
        line_number_right_ = kGutterMiniDiffWidth +
                             std::ceil(kGutterLeftPadding + kLineNumberDigits * zero_advance);
        gutter_width_ = line_number_right_ + kGutterNumberPadding + kGutterMiniDiffWidth +
                        kGutterMargin + kGutterFoldButtonWidth;
        for (size_t i = 0; i < kSourceLines.size(); ++i) {
            highlighted_lines_[i] = highlight_line(body_shaper, kSourceLines[i]);
        }
        line_number_layouts_.resize(kDocumentLineCount);
        for (size_t i = 0; i < kDocumentLineCount; ++i) {
            line_number_layouts_[i] = prepare_retained_text(body_shaper, std::to_string(i + 1));
        }
        if (!find_query_.empty()) {
            set_find_query(find_query_);
        }
    }

    void adjust_body_font_size(float delta) {
        double previous_line_height = line_height_;
        set_body_font_size(body_font_size_ + delta);
        // Keep the same line at the top of the document, whichever view is showing.
        scroll_.jump_to(scroll_.offset() * line_height_ / previous_line_height,
                        std::max(0.0, document_height() - content_bottom()));
        window_->set_animating(scroll_.animating());
        window_->mark_dirty();
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
        context->draw_rect(layout.panel, palette().find_panel_background);
        for (size_t i = 0; i < layout.toggles.size(); ++i) {
            context->draw_rect(layout.toggles[i], find_toggle_on_[i]
                                                      ? palette().find_toggle_on_background
                                                      : palette().find_button_background);
        }
        context->draw_rect(layout.input, palette().find_input_background);
        for (const rect& button : layout.buttons) {
            context->draw_rect(button, palette().find_button_background);
        }
        const double caret_x = layout.input.x + kFindTextInset + find_query_layout_.advance;
        context->draw_rect(rect{caret_x, layout.input.y + 5.0, 1.0, kFindItemHeight - 10.0},
                           palette().find_label);
        context->end_rect_batch();

        context->begin_text_batch();
        draw_centered(context, sidebar_font_, sidebar_metrics_, layout.close, palette().find_muted,
                      &find_close_layout_);
        for (size_t i = 0; i < layout.toggles.size(); ++i) {
            draw_centered(context, sidebar_font_, sidebar_metrics_, layout.toggles[i],
                          palette().find_label, &find_toggle_layouts_[i]);
        }
        for (size_t i = 0; i < layout.buttons.size(); ++i) {
            draw_centered(context, sidebar_font_, sidebar_metrics_, layout.buttons[i],
                          palette().find_label, &find_button_layouts_[i]);
        }
        if (!find_query_.empty()) {
            const double baseline = layout.input.y +
                                    (kFindItemHeight - body_metrics_.line_height) * 0.5 +
                                    body_metrics_.ascent;
            draw_layout(context, body_font_, vec2{layout.input.x + kFindTextInset, baseline},
                        palette().find_label, &find_query_layout_);
            const double count_baseline = layout.input.y +
                                          (kFindItemHeight - sidebar_metrics_.line_height) * 0.5 +
                                          sidebar_metrics_.ascent;
            draw_layout(context, sidebar_font_,
                        vec2{layout.input.right() - kFindTextInset - find_count_layout_.advance,
                             count_baseline},
                        palette().find_muted, &find_count_layout_);
        }
        context->end_text_batch();
    }

    void draw_status_bar(px_render_context* context) {
        const vec2 size = size_;
        const rect bar{0.0, size.y - kStatusBarHeight, size.x, kStatusBarHeight};
        context->begin_rect_batch();
        context->draw_rect(bar, palette().status_bar_background);
        context->end_rect_batch();

        const double baseline =
            bar.y + (bar.h - status_metrics_.line_height) * 0.5 + status_metrics_.ascent;
        context->begin_text_batch();
        draw_layout(context, status_font_, vec2{kStatusBarPadding, baseline},
                    palette().status_bar_text, &status_layout_);
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

    // From the top of the first line's box, which sits `ascent` above its baseline at kTextTop,
    // to the bottom of the last line's, so the last line ends flush with the viewport when
    // scrolled to the end.
    double document_height() const {
        return kTextTop - body_metrics_.ascent + kDocumentLineCount * line_height_;
    }

    double maximum_scroll_offset() const {
        return std::max(0.0, document_height() - content_bottom());
    }

    rect scrollbar_track() const {
        const vec2 size = size_;
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
        return rect{track.x, track.y + travel * progress, track.w, thumb_height};
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

    // The offset the thumb asks for with the pointer at `pointer_y`, or false when the thumb
    // fills the track and cannot move.
    bool thumb_target(double pointer_y, double* offset) const {
        rect track = scrollbar_track();
        rect thumb = scrollbar_thumb();
        double travel = track.h - thumb.h;
        if (travel <= 0.0) {
            return false;
        }
        double thumb_y =
            std::clamp(pointer_y - scrollbar_drag_offset_, track.y, track.bottom() - thumb.h);
        *offset = (thumb_y - track.y) / travel * maximum_scroll_offset();
        return true;
    }

    // A click in the track or a drag of the thumb: the thumb jumps under the pointer.
    void scroll_thumb_to(double pointer_y) {
        double offset = 0.0;
        if (thumb_target(pointer_y, &offset)) {
            jump_scroll_to(offset);
        }
    }

    window* window_ = nullptr;
    px_font_t* body_font_ = nullptr;
    float body_font_size_ = 0.0f;
    px_font_t* sidebar_title_font_ = nullptr;
    px_font_t* sidebar_font_ = nullptr;
    px_font_t* status_font_ = nullptr;
    double line_height_ = 0.0;
    // Both measured from the gutter's left edge: where the line numbers end, and where the
    // text area begins.
    double line_number_right_ = 0.0;
    double gutter_width_ = 0.0;
    px_font_metrics body_metrics_;
    px_font_metrics sidebar_title_metrics_;
    px_font_metrics sidebar_metrics_;
    px_font_metrics status_metrics_;
    smooth_scroll scroll_;
    bool sidebar_visible_ = true;
    bool find_panel_visible_ = false;
    bool highlight_gutter_ = false;
    bool dark_mode_ = false;
    // The editor's area, kept from the resize events and the bounds it paints with.
    vec2 size_;
    std::string find_query_;
    size_t find_match_count_ = 0;
    PreparedText find_query_layout_;
    PreparedText find_count_layout_;
    PreparedText find_close_layout_;
    PreparedText status_layout_;
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

    px_font_t* sidebar_title_font = px_create_font("system", kSidebarTitleFontSize, PX_FONT_BOLD);
    px_font_t* sidebar_font = px_create_font("system", kSidebarFontSize);
    // EDITOR_FONT_SIZE starts the body font at that size.
    float body_font_size = kMainFontSize;
    if (const char* requested = std::getenv("EDITOR_FONT_SIZE")) {
        float parsed = std::strtof(requested, nullptr);
        if (parsed > 0.0f) {
            body_font_size = parsed;
        }
    }
    EditorControl root(&window, sidebar_title_font, sidebar_font, body_font_size);
    window.set_root_control(&root);
    window.set_maximized(true);
    window.show();
    px_run_event_loop();
    return 0;
}
