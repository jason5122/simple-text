#include "experiments/platform/px/grapheme_shaper.h"
#include "experiments/platform/px/px.h"

#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

constexpr double kWindowWidth = 980.0;
constexpr double kWindowHeight = 620.0;
constexpr fcolor kBackground{1.0f, 1.0f, 1.0f, 1.0f};
constexpr fcolor kHeadingColor{0.10f, 0.12f, 0.16f, 1.0f};
constexpr fcolor kTextColor{0.18f, 0.22f, 0.28f, 1.0f};
constexpr fcolor kHexColor{0.05f, 0.35f, 0.70f, 1.0f};
constexpr fcolor kNameColor{0.58f, 0.20f, 0.10f, 1.0f};

std::u32string make_line(std::u32string_view prefix,
                         std::initializer_list<char32_t> codepoints) {
    std::u32string result(prefix);
    for (char32_t codepoint : codepoints) {
        result.push_back(codepoint);
        result.push_back(U' ');
    }
    return result;
}

struct Example {
    std::u32string text;
    double baseline;
};

class ControlCharDemo final : public px_window_event_handler {
public:
    ControlCharDemo()
        : heading_font_(px_create_font("system", 20.0f, PX_FONT_BOLD)),
          body_font_(px_create_font("system", 15.0f)),
          hex_shaper_(std::make_unique<grapheme_shaper>(body_font_)),
          name_shaper_(std::make_unique<grapheme_shaper>(
              body_font_, GRAPHEME_SHAPER_DRAW_COMMON_WHITESPACE | GRAPHEME_SHAPER_DRAW_BIDI |
                              GRAPHEME_SHAPER_DRAW_ALL_WHITESPACE |
                              GRAPHEME_SHAPER_USE_CONTROL_NAMES)) {
        hex_examples_ = {
            {make_line(U"C0 low:   ", {0x00, 0x01, 0x02, 0x07, 0x08}), 135.0},
            {make_line(U"C0 high:  ", {0x0b, 0x0c, 0x0d, 0x1b, 0x1f}), 175.0},
            {make_line(U"DEL / C1: ", {0x7f, 0x80, 0x85, 0x8d, 0x9f}), 215.0},
        };
        name_examples_ = {
            {make_line(U"C0 low:   ", {0x00, 0x01, 0x02, 0x07, 0x08}), 350.0},
            {make_line(U"DEL / C1: ", {0x7f, 0x80, 0x85, 0x8d, 0x9f}), 390.0},
            {make_line(U"Unicode:  ", {0x00a0, 0x00ad, 0x061c, 0x200b, 0x200e, 0x2060, 0x3000}),
             430.0},
        };
    }

    void attach(px_window_t* window) { window_ = window; }

    bool handle_event(px_event_t* event) override {
        if (event->type == PX_EVENT_KEY && event->pressed && event->key == PX_KEY_ESCAPE) {
            px_close_window(window_);
            return true;
        }
        return false;
    }

    void paint(px_render_context* context, rect bounds, const rect*, int) override {
        context->draw_rect(bounds, kBackground);

        context->begin_text_batch();
        context->draw_text(heading_font_, {48.0, 54.0}, kHeadingColor,
                           "Unicode control-character labels", true);
        context->draw_text(body_font_, {48.0, 92.0}, kTextColor,
                           "Default policy: the input below contains raw control code points.",
                           true);
        draw_examples(context, hex_shaper_.get(), hex_examples_, kHexColor);

        context->draw_text(body_font_, {48.0, 292.0}, kTextColor,
                           "Mnemonic policy, including optional whitespace and bidi markers:",
                           true);
        draw_examples(context, name_shaper_.get(), name_examples_, kNameColor);

        context->draw_text(body_font_, {48.0, 510.0}, kTextColor,
                           "Tab and newline are intentionally not converted into labels.", true);
        context->end_text_batch();
    }

private:
    static void draw_examples(px_render_context* context,
                              grapheme_shaper* shaper,
                              const std::vector<Example>& examples,
                              color value) {
        for (const Example& example : examples) {
            shaper->draw_string(context, 48.0, example.baseline, value, example.text, false, {},
                                false, 0.0f, 0.0f);
        }
    }

    px_window_t* window_ = nullptr;
    px_font_t* heading_font_ = nullptr;
    px_font_t* body_font_ = nullptr;
    std::unique_ptr<grapheme_shaper> hex_shaper_;
    std::unique_ptr<grapheme_shaper> name_shaper_;
    std::vector<Example> hex_examples_;
    std::vector<Example> name_examples_;
};

}  // namespace

int main(int argc, char** argv) {
    px_init("control-char-demo", "com.example.control-char-demo", argc, argv, 0);

    ControlCharDemo demo;
    px_window_t* window = px_create_window(&demo, nullptr, kWindowWidth, kWindowHeight,
                                           "control character demo", kBackground,
                                           PX_WINDOW_DEFAULT);
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
