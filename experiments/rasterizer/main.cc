#include "base/apple/scoped_cftyperef.h"
#include "base/apple/scoped_cgtyperef.h"
#include "base/numeric/safe_conversions.h"
#include "base/unicode/unicode.h"
#include "experiments/rasterizer/font.h"
#include "experiments/rasterizer/gl_helpers.h"
#include "experiments/rasterizer/mac_helpers.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <spdlog/spdlog.h>
#include <string_view>
#include <vector>

using base::apple::ScopedCFTypeRef;
using base::apple::ScopedCGColorSpace;
using base::apple::ScopedCGContext;
using base::apple::ScopedCGImage;

namespace {

void draw_glyph(base::apple::ScopedCGContext ctx,
                font::GlyphRasterizer& rasterizer,
                const font::FontHandle& run_font,
                font::GlyphId glyph_id,
                double glyph_x,  // points
                double glyph_y,  // points
                double scale,
                bool use_subpixel_positioning) {
    constexpr double kEpsilon = 1e-6;
    const double device_x_f = glyph_x * scale + kEpsilon;
    int device_x = base::clamp_round<int>(device_x_f);
    double subpixel_x = 0.0;
    if (use_subpixel_positioning) {
        device_x = base::clamp_floor<int>(device_x_f);
        const double frac = device_x_f - device_x;
        const int phase = base::clamp_floor<int>(frac * 6.0);
        subpixel_x = phase * scale / 6.0;
    }

    font::GlyphBitmap bmp = rasterizer.rasterize(run_font, glyph_id, scale, subpixel_x);
    if (bmp.empty()) return;

    // +2 / +124 are debug margins to line up with the Sublime capture.
    const int dst_x = device_x + bmp.bearing_x + 2;
    const int dst_y = base::clamp_round<int>(glyph_y * scale) + bmp.bearing_y + 124;
    BitmapView img = {
        .pixels = bmp.pixels,
        .width = bmp.width,
        .height = bmp.height,
    };
    blit_pixels(ctx, img, dst_x, dst_y);
}

void draw_text(base::apple::ScopedCGContext ctx,
               const font::TextShaper& shaper,
               const font::FontHandle& font,
               std::string_view utf8,
               size_t line_index,
               double scale,
               bool use_subpixel_positioning) {
    const double ascent = std::ceil(font.ascent());
    const double line_height = ascent + std::ceil(font.descent()) + std::ceil(font.leading());
    const double baseline_y = ascent + line_height * line_index;

    const font::ShapedLine shaped = shaper.shape(font, utf8);
    font::GlyphRasterizer rasterizer;
    for (const auto& run : shaped.runs) {
        for (const auto& g : run.glyphs) {
            draw_glyph(ctx, rasterizer, run.font, g.glyph_id, g.x_offset, baseline_y + g.y_offset,
                       scale, use_subpixel_positioning);
        }
    }
}

// The 32 printable-ASCII punctuation characters Sublime builds programming ligatures from (==, !=,
// ->, ...). This is exactly Sublime's documented set, and matches the four-range check in its
// binary: every printable ASCII character except space, the digits, and the letters. Those three
// excluded groups partition the punctuation into four contiguous ranges:
//
//   0x21-0x2F  ! " # $ % & ' ( ) * + , - . /   (between space and the digits)
//   0x3A-0x40  : ; < = > ? @                   (between the digits and A-Z)
//   0x5B-0x60  [ \ ] ^ _ `                     (between A-Z and a-z)
//   0x7B-0x7E  { | } ~                         (between a-z and DEL)
bool is_ascii_operator(base::Unichar cp) {
    return (cp >= 0x21 && cp <= 0x2F) || (cp >= 0x3A && cp <= 0x40) ||
           (cp >= 0x5B && cp <= 0x60) || (cp >= 0x7B && cp <= 0x7E);
}

// Only monospace fonts get operator ligatures.
bool is_monospace(const font::TextShaper& shaper, const font::FontHandle& handle) {
    return std::abs(shaper.shape(handle, "i").width - shaper.shape(handle, "M").width) < 0.01;
}

// Like draw_text, but shapes and positions each codepoint on its own (Sublime's context-free
// model) and accumulates the pen from per-codepoint advances instead of using CTLine's line-global
// positions. For the Latin+CJK corpus this should be pixel-identical to draw_text; it diverges
// only where context matters (combining marks, ligatures, complex scripts). No caching yet.
void draw_text_2(base::apple::ScopedCGContext ctx,
                 const font::TextShaper& shaper,
                 const font::FontHandle& font,
                 std::string_view utf8,
                 size_t line_index,
                 double scale,
                 bool use_subpixel_positioning) {
    // Combining marks (non-base characters) attach to the preceding base, so a grapheme cluster --
    // base plus its trailing marks -- must be shaped as one unit. Shaped standalone, each mark
    // gets a real advance and spreads instead of stacking. Runs with no marks are single
    // codepoints, so Latin/CJK are unchanged (CJK aren't non-base). This is Sublime's
    // grapheme-boundary exception.
    CFCharacterSetRef nonbase = CFCharacterSetGetPredefined(kCFCharacterSetNonBase);
    const bool monospace = is_monospace(shaper, font);

    // A codepoint is a combining mark if it's in the non-base set; an invalid codepoint (-1)
    // isn't.
    auto is_mark = [&](base::Unichar cp) {
        return cp >= 0 && CFCharacterSetIsLongCharacterMember(nonbase, static_cast<UTF32Char>(cp));
    };

    font::GlyphRasterizer rasterizer;
    double pen = 0.0;  // accumulated advance in points
    const double ascent = std::ceil(font.ascent());
    const double line_height = ascent + std::ceil(font.descent()) + std::ceil(font.leading());
    const double baseline_y = ascent + line_height * line_index;

    for (size_t i = 0; i < utf8.size();) {
        size_t cluster_end = i;
        const base::Unichar base_cp =
            base::next_utf8(utf8, cluster_end);  // advances past the base

        // Extend the cluster. Combining marks take precedence (matching ST): a base plus its
        // trailing non-base marks is one grapheme. Otherwise, in a monospace font, a run of ASCII
        // operators shapes together so CoreText forms programming ligatures (==, !=, ->, ...).
        size_t peek = cluster_end;
        const bool next_is_mark =
            cluster_end < utf8.size() && is_mark(base::next_utf8(utf8, peek));
        if (next_is_mark) {
            while (cluster_end < utf8.size()) {
                size_t next = cluster_end;
                if (!is_mark(base::next_utf8(utf8, next))) break;
                cluster_end = next;
            }
        } else if (monospace && is_ascii_operator(base_cp)) {
            while (cluster_end < utf8.size()) {
                size_t next = cluster_end;
                if (!is_ascii_operator(base::next_utf8(utf8, next))) break;
                cluster_end = next;
            }
        }

        const font::ShapedLine shaped = shaper.shape(font, utf8.substr(i, cluster_end - i));
        i = cluster_end;

        const double origin_x = pen;
        for (const auto& run : shaped.runs) {
            for (const auto& g : run.glyphs) {
                draw_glyph(ctx, rasterizer, run.font, g.glyph_id, origin_x + g.x_offset,
                           baseline_y + g.y_offset, scale, use_subpixel_positioning);
                pen += g.x_advance;
            }
        }
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    // Disable stdout buffering.
    std::setbuf(stdout, nullptr);

    std::vector<std::string> lines = {
        "",
        "",
        "",
        "Sphinx of black quartz, judge my vow!",
        "The quick brown fox jumps over the lazy dog. 你好",
        "",
        "Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod",
        "tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam,",
        "quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo",
        "consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse",
        "cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non",
        "proident, sunt in culpa qui officia deserunt mollit anim id est laborum.",
    };
    auto family = std::string(argv[1]);
    double font_size = std::stoi(argv[2]);
    // Trailing style args, any order: "bold", "italic". Applied as traits in create_font().
    font::Weight weight = font::Weight::Normal;
    font::Slant slant = font::Slant::Normal;
    for (int i = 3; i < argc; i++) {
        std::string_view style = argv[i];
        if (style == "bold") weight = font::Weight::Bold;
        else if (style == "italic") slant = font::Slant::Italic;
    }
    if (family == "Apple Color Emoji") {
        lines = {
            "",
            "",
            "",
            "😀 😃 😄 😁 😆 😅 😂 🤣 🥲 ☺️ 😊 😇 🙂 🙃 😉",
            "😌 😍 🥰 😘 😗 😙 😚 😋 😛 😝 😜 🤪 🤨 🧐 🤓",
            "😎 🤩 🥳 😏 😒 😞 😔 😟 😕 🙁 😣 😖 😫 😩 🥺",
            "😢 😭 😤 😠 😡 🤯 😳 🥵 🥶 😱 😨 😰 😥 😓 🤔",
            "🤫 🤭 🥱 😴 🤤 😷 🤒 🤕 🤢 🤮 🤧 😇 🤠 🤡 🫪",
        };
    }
    if (family == "Geeza Pro") {
        lines = {
            "", "꣰", "ᩣᩤᩥᩦᩧᩨᩩᩪᩫᩬᩭ", "⃒⃓⃘⃙⃚⃑⃔⃕⃖⃗⃛⃜⃝⃞⃟⃠⃡⃢⃣⃤⃥", "̴̵̶̷̸̡̢̧̨̣̤̥̦̩̪̫̬̭̮̯̰̱̲̳̹̺̻̼͇͈͉͍͎̽̾̿̀́͂̓̈́͆͊͋͌ͅ͏͓͔͕͖͙͚͐͑͒͗͛ͣͤͥͦͧͨͩͪͫͬͭͮͯ͘͜͟͢͝͞͠͡Ͱ",
        };
    }
    if (family == "Fira Code") {
        lines = {
            "",
            "fi == !=",
            "🇺🇸 🇯🇵 🇪🇺",
            "👨‍👩‍👧‍👦 🏴‍☠️ 👩‍❤️‍💋‍👨",
            "👍🏽 👩🏽‍🦰 👩🏾‍👨🏼‍👧🏽‍👦🏻",
        };
    }

    constexpr size_t width = 2000;
    constexpr size_t height = 1000;
    constexpr double scale = 2.0;

    constexpr bool kUseIndividualShaping = true;
    constexpr bool kUseOpenGL = true;

    font::FontDatabase db;
    auto face = db.match({family, weight, slant});
    if (!face) return 1;
    auto handle = db.create_font(*face, font_size);
    if (!handle) return 1;

    font::TextShaper shaper;

    auto ctx = create_context(width, height);
    const bool use_subpixel_positioning = font_size <= font::kSmallSizeThresholdPt;
    for (size_t i = 0; i < lines.size(); i++) {
        if (kUseIndividualShaping) {
            draw_text_2(ctx, shaper, *handle, lines[i], i, scale, use_subpixel_positioning);
        } else {
            draw_text(ctx, shaper, *handle, lines[i], i, scale, use_subpixel_positioning);
        }
    }

    if (kUseOpenGL) {
        show_window_gl(ctx, scale);
    } else {
        show_window(ctx, scale);
    }
}
