#include "base/numeric/safe_conversions.h"
#include "base/unicode/unicode.h"
#include "experiments/rasterizer/layout.h"
#include <cmath>
#include <string_view>
#include <uni_algo/prop.h>

namespace {

void draw_glyph(GlyphAtlasSource& out,
                const font::FontHandle& run_font,
                font::GlyphId glyph_id,
                double glyph_x,  // points
                double glyph_y,  // points
                double scale) {
    // ST floors this with no epsilon, but its pen lands on/above each integer boundary where ours
    // drifts a hair below: summing the per-glyph advance (`pen += advance`) accumulates rounding
    // error, so by ~glyph 35 on a long line the value dips just under the boundary (e.g.
    // 692.99999999999977 vs 693.0). A bare floor would drop those glyphs to the pixel below (phase
    // 5). This nudge absorbs that sub-ULP drift; it's orders of magnitude smaller than a real
    // sub-pixel gap, so it never affects a glyph that isn't already on a boundary.
    constexpr double kEpsilon = 1e-6;
    const double device_x_f = glyph_x * scale + kEpsilon;
    const int device_x = base::clamp_floor<int>(device_x_f);
    const double frac = device_x_f - device_x;
    const int phase = base::clamp_floor<int>(frac * 6.0);
    const double subpixel_x = phase * scale / 6.0;

    const GlyphKey key = {run_font.cache_key(), glyph_id, phase};
    auto it = out.bitmaps.find(key);
    if (it == out.bitmaps.end()) {
        it =
            out.bitmaps.emplace(key, font::rasterize(run_font, glyph_id, scale, subpixel_x)).first;
    }
    const font::GlyphBitmap& bmp = it->second;
    if (bmp.empty()) return;

    // +2 / +124 are debug margins to line up with the Sublime capture.
    const int dst_x = device_x + bmp.bearing_x + 2;
    const int dst_y = base::clamp_round<int>(glyph_y * scale) + bmp.bearing_y + 68;
    out.instances.push_back({.key = key, .dst_x = dst_x, .dst_y = dst_y});
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

// A combining mark (general category Mn/Mc/Me). Sublime -- like Core Text's non-base set -- stacks
// every combining mark onto the preceding base, so marks must shape as one cluster with it.
bool is_mark(base::Unichar cp) {
    if (cp < 0) return false;
    const una::codepoint::prop p{static_cast<char32_t>(cp)};
    return p.General_Category_Mn() || p.General_Category_Mc() || p.General_Category_Me();
}

// is_mark() (General_Category Mn/Mc/Me) undershoots what ST's is_combining_char actually merges:
// empirically (see the COMPARE_GRAPHEME harness that validated this against uni_algo across
// scripts.txt), skin-tone modifiers and variation selectors merge too, even though neither is
// Mn/Mc/Me -- they're Unicode's "Other_Grapheme_Extend" additions to the real
// Grapheme_Cluster_Break=Extend property, which is what a binary-search range table like ST's
// is_combining_char more plausibly implements.
bool st_is_extend(base::Unichar cp) {
    return is_mark(cp) || (cp >= 0x1F3FB && cp <= 0x1F3FF)  // Emoji_Modifier (skin tone)
           || (cp >= 0xFE00 && cp <= 0xFE0F);               // variation selectors (VS1-16)
}

// ST's real grapheme boundary, disassembled at 0x1000c5670 in the portable Sublime Text.app binary
// (`./Sublime Text.app`, not /Applications' -- a different build). It's a simpler loop than UAX
// #29: starting from a base codepoint, extend the cluster while the next codepoint is a combining
// mark (ST's is_combining_char -- see st_is_extend), the second half of a regional-indicator pair
// (flag emoji), or a ZWJ (which also pulls in whatever follows it). Otherwise stop. Notably
// absent: Hangul jamo composition (GB6-8) and Unicode 15.1's GB9c Indic-conjunct lookahead -- ST
// just doesn't have either, so e.g. Devanagari स्ते splits into स् + ते rather than staying one
// cluster. Byte length of the first cluster in `s`.
size_t st_grapheme_boundary(std::string_view s) {
    if (s.empty()) return 0;

    auto is_regional_indicator = [](base::Unichar cp) { return cp >= 0x1F1E6 && cp <= 0x1F1FF; };

    size_t i = 0;
    bool ri_armed = is_regional_indicator(base::next_utf8(s, i));

    while (i < s.size()) {
        size_t next = i;
        const base::Unichar cp = base::next_utf8(s, next);

        if (ri_armed && is_regional_indicator(cp)) {
            i = next;
            ri_armed = false;
            continue;
        }
        ri_armed = false;

        if (st_is_extend(cp)) {
            i = next;
            continue;
        }

        if (cp == 0x200D) {  // ZWJ joins to whatever follows it too, sight unseen.
            i = next;
            if (i < s.size()) base::next_utf8(s, i);
            continue;
        }

        break;
    }
    return i;
}

void draw_cluster(GlyphAtlasSource& out,
                  const font::FontHandle& font,
                  std::string_view cluster,
                  double baseline_y,
                  double scale,
                  double& pen) {
    const double origin_x = pen;
    const auto shaped = font::shape(font, cluster);
    double delta = 0.0;
    for (const auto& run : shaped) {
        // Sublime's shaper snaps monospace advances to whole points at 16.0pt and below.
        // TODO: Should we hoist this calculation into main()?
        const bool snap = font.is_monospace() && font.size() <= 16.0;

        for (const auto& g : run.glyphs) {
            const double x_advance = snap ? std::round(g.x_advance) : g.x_advance;
            delta += x_advance - g.x_advance;
            draw_glyph(out, run.font, g.glyph_id, origin_x + g.x_offset + delta,
                       baseline_y + g.y_offset, scale);
            pen += x_advance;
        }
    }
}

void draw_text(GlyphAtlasSource& out,
               const font::FontHandle& font,
               std::string_view utf8,
               size_t line_index,
               double scale) {
    const double ascent = std::ceil(font.ascent());
    const double descent = std::ceil(font.descent());
    const double leading = std::ceil(font.leading());
    const double line_height = ascent + descent + leading;
    const double baseline_y = ascent + line_height * line_index;
    const bool monospace = font.is_monospace();

    double pen = 0.0;  // accumulated advance in points
    for (size_t i = 0; i < utf8.size();) {
        // ST's own grapheme boundary (see st_grapheme_boundary), not UAX #29: combining marks,
        // regional-indicator flag pairs, and ZWJ sequences stay together and the shaper composes
        // them, but a virama-joined Indic conjunct doesn't -- ST breaks it apart, so we do too.
        size_t cluster_end = i + st_grapheme_boundary(utf8.substr(i));

        // Operator ligatures ride on top of grapheme clustering (neither ST nor UAX #29 merges
        // "=="): in a monospace font a run of lone ASCII operators shapes together so the font
        // forms
        // ==, !=, ->. Each operator is its own single-codepoint grapheme (base_end ==
        // cluster_end), so this never disturbs a marks/emoji cluster.
        size_t base_end = i;
        const base::Unichar base_cp = base::next_utf8(utf8, base_end);
        if (monospace && base_end == cluster_end && is_ascii_operator(base_cp)) {
            while (cluster_end < utf8.size()) {
                size_t next = cluster_end;
                if (!is_ascii_operator(base::next_utf8(utf8, next))) break;
                cluster_end = next;
            }
        }

        draw_cluster(out, font, utf8.substr(i, cluster_end - i), baseline_y, scale, pen);
        i = cluster_end;
    }
}

}  // namespace

GlyphAtlasSource layout_text(const font::FontHandle& font,
                             const std::vector<std::string>& lines,
                             double scale) {
    GlyphAtlasSource source;
    for (size_t i = 0; i < lines.size(); i++) draw_text(source, font, lines[i], i, scale);
    return source;
}
