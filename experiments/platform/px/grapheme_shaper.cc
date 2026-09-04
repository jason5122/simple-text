#include "experiments/platform/px/grapheme_shaper.h"

#include "base/numeric/safe_conversions.h"
#include "base/unicode/unicode.h"
#include "experiments/platform/px/px_font_internal.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <map>
#include <string>
#include <uni_algo/prop.h>
#include <utility>

namespace {

constexpr size_t kMaximumBatchGlyphs = 32;

constexpr std::array<const char*, 32> kC0Names = {
    "NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL", "BS",  "HT",  "LF",
    "VT",  "FF",  "CR",  "SO",  "SI",  "DLE", "DC1", "DC2", "DC3", "DC4", "NAK",
    "SYN", "ETB", "CAN", "EM",  "SUB", "ESC", "FS",  "GS",  "RS",  "US",
};

constexpr std::array<const char*, 33> kC1Names = {
    "DEL", "PAD", "HOP", "BPH", "NBH",  "IND", "NEL", "SSA", "ESA", "HTS", "HTJ",
    "VTS", "PLD", "PLU", "RI",  "SS2",  "SS3", "DCS", "PU1", "PU2", "STS", "CCH",
    "MW",  "SPA", "EPA", "SOS", "SGCI", "SCI", "CSI", "ST",  "OSC", "PM",  "APC",
};

std::u32string ascii_to_utf32(std::string_view input) {
    std::u32string result;
    result.reserve(input.size());
    for (const unsigned char c : input) {
        result.push_back(static_cast<char32_t>(c));
    }
    return result;
}

bool is_ascii_operator(base::Unichar cp) {
    return (cp >= 0x21 && cp <= 0x2f) || (cp >= 0x3a && cp <= 0x40) ||
           (cp >= 0x5b && cp <= 0x60) || (cp >= 0x7b && cp <= 0x7e);
}

bool is_regional_indicator(char32_t codepoint) {
    return codepoint >= 0x1f1e6 && codepoint <= 0x1f1ff;
}

bool is_combining_char(char32_t codepoint) {
    if (!base::is_valid_codepoint(static_cast<base::Unichar>(codepoint))) {
        return false;
    }
    const una::codepoint::prop property{codepoint};
    return property.General_Category_Mn() || property.General_Category_Mc() ||
           property.General_Category_Me() || (codepoint >= 0x1f3fb && codepoint <= 0x1f3ff) ||
           (codepoint >= 0xfe00 && codepoint <= 0xfe0f);
}

bool is_sara_am(char32_t codepoint) { return codepoint == 0x0e33 || codepoint == 0x0eb3; }

char32_t measurement_codepoint(char32_t codepoint) {
    if (codepoint == U'\n') {
        return U' ';
    }
    if (is_sara_am(codepoint)) {
        return codepoint - 1;
    }
    return codepoint;
}

bool is_trivial_grapheme(char32_t first, char32_t second) {
    return !(is_regional_indicator(first) && is_regional_indicator(second)) &&
           !is_combining_char(second) && second != 0x200d && !is_sara_am(second);
}

size_t utf8_cluster_to_utf32_index(std::u32string_view text, size_t byte_offset) {
    size_t bytes = 0;
    for (size_t index = 0; index < text.size(); ++index) {
        if (bytes >= byte_offset) {
            return index;
        }
        const uint32_t cp = static_cast<uint32_t>(text[index]);
        bytes += cp <= 0x7f ? 1 : cp <= 0x7ff ? 2 : cp <= 0xffff ? 3 : 4;
    }
    return text.size();
}

struct utf8_decoder {
    static constexpr bool supports_space_markers = false;

    static size_t next_codepoint(std::string_view text, size_t start, char32_t* codepoint) {
        size_t end = start;
        *codepoint = static_cast<char32_t>(base::next_utf8(text, end));
        return end;
    }

    static size_t next_grapheme_boundary(std::string_view text,
                                         size_t start,
                                         size_t first_end,
                                         char32_t first) {
        if (first_end >= text.size()) {
            return first_end;
        }

        size_t second_end = first_end;
        const char32_t second = static_cast<char32_t>(base::next_utf8(text, second_end));
        if (is_trivial_grapheme(first, second)) {
            return first_end;
        }

        size_t end = first_end;
        if (is_regional_indicator(first) && is_regional_indicator(second)) {
            end = second_end;
        }
        while (end < text.size()) {
            size_t next = end;
            const char32_t codepoint = static_cast<char32_t>(base::next_utf8(text, next));
            if (is_combining_char(codepoint)) {
                end = next;
                continue;
            }
            if (codepoint == 0x200d) {
                end = next;
                if (end < text.size()) {
                    base::next_utf8(text, end);
                }
                continue;
            }
            break;
        }

        if (end < text.size()) {
            size_t next = end;
            if (is_sara_am(static_cast<char32_t>(base::next_utf8(text, next)))) {
                end = next;
            }
        }
        return end;
    }

    static size_t source_cluster(std::string_view, size_t start, size_t native_cluster) {
        return start + native_cluster;
    }
};

struct utf32_decoder {
    static constexpr bool supports_space_markers = true;

    static size_t next_codepoint(std::u32string_view text, size_t start, char32_t* codepoint) {
        *codepoint = text[start];
        return start + 1;
    }

    static size_t next_grapheme_boundary(std::u32string_view text,
                                         size_t start,
                                         size_t first_end,
                                         char32_t first) {
        if (first_end >= text.size()) {
            return first_end;
        }
        const char32_t second = text[first_end];
        if (is_trivial_grapheme(first, second)) {
            return first_end;
        }

        size_t end = first_end;
        if (is_regional_indicator(first) && is_regional_indicator(second)) {
            ++end;
        }
        while (end < text.size()) {
            const char32_t codepoint = text[end];
            if (is_combining_char(codepoint)) {
                ++end;
                continue;
            }
            if (codepoint == 0x200d) {
                ++end;
                if (end < text.size()) {
                    ++end;
                }
                continue;
            }
            break;
        }
        if (end < text.size() && is_sara_am(text[end])) {
            ++end;
        }
        return end;
    }

    static size_t source_cluster(std::u32string_view cluster,
                                 size_t start,
                                 size_t native_cluster) {
        return start + utf8_cluster_to_utf32_index(cluster, native_cluster);
    }
};

const char* control_name(char32_t codepoint) {
    const uint32_t cp = static_cast<uint32_t>(codepoint);
    if (cp < kC0Names.size()) {
        return kC0Names[cp];
    }
    if (cp >= 0x7f && cp <= 0x9f) {
        return kC1Names[cp - 0x7f];
    }
    // The remaining 54-entry table is stored by code point in Build 4200. These abbreviations are
    // copied from its UTF-32 string data rather than synthesized from Unicode character names.
    switch (cp) {
    case 0x00a0:
        return "NBSP";
    case 0x00ad:
        return "SHY";
    case 0x061c:
        return "ALM";
    case 0x115f:
        return "HCFIL";
    case 0x1160:
        return "HJFIL";
    case 0x17b4:
        return "KVIAQ";
    case 0x17b5:
        return "KVIAA";
    case 0x180e:
        return "MVS";
    case 0x2000:
        return "ENQUAD";
    case 0x2001:
        return "EMQUAD";
    case 0x2002:
        return "ENSP";
    case 0x2003:
        return "EMSP";
    case 0x2004:
        return "EMSP13";
    case 0x2005:
        return "EMSP14";
    case 0x2006:
        return "EMSP16";
    case 0x2007:
        return "NUMSP";
    case 0x2008:
        return "PUNCSP";
    case 0x2009:
        return "THINSP";
    case 0x200a:
        return "HAIRSP";
    case 0x200b:
        return "ZWSP";
    case 0x200c:
        return "ZWNJ";
    case 0x200d:
        return "ZWJ";
    case 0x200e:
        return "LRM";
    case 0x200f:
        return "RLM";
    case 0x2028:
        return "LSEP";
    case 0x2029:
        return "PSEP";
    case 0x202a:
        return "LRE";
    case 0x202b:
        return "RLE";
    case 0x202c:
        return "PDF";
    case 0x202d:
        return "LRO";
    case 0x202e:
        return "RLO";
    case 0x202f:
        return "NNBSP";
    case 0x205f:
        return "MMSP";
    case 0x2060:
        return "WJ";
    case 0x2061:
        return "FUNA";
    case 0x2062:
        return "ITIM";
    case 0x2063:
        return "ISEP";
    case 0x2064:
        return "IPLUS";
    case 0x2066:
        return "LRI";
    case 0x2067:
        return "RLI";
    case 0x2068:
        return "FSI";
    case 0x2069:
        return "PDI";
    case 0x206a:
        return "ISSW";
    case 0x206b:
        return "ASSW";
    case 0x206c:
        return "IAFS";
    case 0x206d:
        return "AAFS";
    case 0x206e:
        return "NATDS";
    case 0x206f:
        return "NOMDS";
    case 0x2800:
        return "DOTS0";
    case 0x3000:
        return "FWSP";
    case 0x3164:
        return "HFIL";
    case 0xfeff:
        return "BOM";
    case 0xffa0:
        return "HHFIL";
    default:
        return nullptr;
    }
}

}  // namespace

std::map<px_font_t*, std::unique_ptr<grapheme_shaper>> grapheme_shaper::s_cache;

grapheme_shaper::grapheme_shaper(px_font_t* font, uint32_t flags)
    : font_(font), flags_(flags), monospace_(px_font_is_monospace(font)) {}

grapheme_shaper::~grapheme_shaper() = default;

bool grapheme_shaper::should_draw_as_control(char32_t codepoint) const {
    const uint32_t cp = static_cast<uint32_t>(codepoint);

    // ST always visualizes C0 except tab/newline, plus DEL and all of C1. CR reaches this function
    // as a control; TextBuffer's line splitting normally keeps it out of a drawable substring.
    if ((cp < 0x20 && cp != 0x09 && cp != 0x0a) || (cp >= 0x7f && cp <= 0x9f)) {
        return true;
    }

    if (flags_ & GRAPHEME_SHAPER_DRAW_COMMON_WHITESPACE) {
        if (cp == 0x00a0 || cp == 0x00ad || (cp >= 0x2000 && cp <= 0x200d) || cp == 0x2028 ||
            cp == 0x2029 || cp == 0x202f || cp == 0x205f || cp == 0x2060 || cp == 0xfeff) {
            return true;
        }
    }

    if (flags_ & GRAPHEME_SHAPER_DRAW_BIDI) {
        if (cp == 0x061c || cp == 0x200e || cp == 0x200f || (cp >= 0x202a && cp <= 0x202e) ||
            (cp >= 0x2066 && cp <= 0x2069)) {
            return true;
        }
    }

    if (flags_ & GRAPHEME_SHAPER_DRAW_ALL_WHITESPACE) {
        if (cp == 0x115f || cp == 0x1160 || cp == 0x17b4 || cp == 0x17b5 || cp == 0x180e ||
            (cp >= 0x2061 && cp <= 0x206f) || cp == 0x2800 || cp == 0x3000 || cp == 0x3164 ||
            cp == 0xffa0) {
            return true;
        }
    }

    return false;
}

std::unique_ptr<fx_layout> grapheme_shaper::shape_control(char32_t codepoint) {
    if (!font_ || !font_->font) {
        return nullptr;
    }

    if (flags_ & GRAPHEME_SHAPER_USE_CONTROL_NAMES) {
        if (const char* name = control_name(codepoint)) {
            const std::string label = std::string("<") + name + ">";
            return font_->font->shape(ascii_to_utf32(label));
        }
    }

    char label[16] = {};
    const uint32_t cp = static_cast<uint32_t>(codepoint);
    if (cp <= 0xff) {
        std::snprintf(label, sizeof(label), "<0x%02x>", cp);
    } else {
        // The Build 4200 binary uses exactly four lowercase nibbles for this path.
        std::snprintf(label, sizeof(label), "<0x%04x>", cp & 0xffffu);
    }
    return font_->font->shape(ascii_to_utf32(label));
}

fx_layout* grapheme_shaper::find_layout(char32_t codepoint) {
    auto found = codepoint_layouts_.find(codepoint);
    if (found != codepoint_layouts_.end()) {
        return found->second.get();
    }

    std::unique_ptr<fx_layout> layout;
    if (font_ && font_->font) {
        if (should_draw_as_control(codepoint)) {
            layout = shape_control(codepoint);
        } else {
            const char32_t text[] = {codepoint};
            layout = font_->font->shape(std::u32string_view(text, 1));
        }
    }
    fx_layout* result = layout.get();
    codepoint_layouts_.emplace(codepoint, std::move(layout));
    return result;
}

fx_layout* grapheme_shaper::find_layout(std::string_view utf8) {
    auto found = utf8_layouts_.find(utf8);
    if (found != utf8_layouts_.end()) {
        return found->second.get();
    }
    std::unique_ptr<fx_layout> layout = font_ && font_->font ? font_->font->shape(utf8) : nullptr;
    auto inserted = utf8_layouts_.emplace(std::string(utf8), std::move(layout)).first;
    return inserted->second.get();
}

fx_layout* grapheme_shaper::find_layout(std::u32string_view utf32) {
    auto found = utf32_layouts_.find(utf32);
    if (found != utf32_layouts_.end()) {
        return found->second.get();
    }
    std::unique_ptr<fx_layout> layout = font_ && font_->font ? font_->font->shape(utf32) : nullptr;
    auto inserted = utf32_layouts_.emplace(std::u32string(utf32), std::move(layout)).first;
    return inserted->second.get();
}

float grapheme_shaper::measure_glyph(char32_t codepoint) {
    fx_layout* layout = find_layout(measurement_codepoint(codepoint));
    return layout ? layout->advance : 0.0f;
}

float grapheme_shaper::measure_glyph(std::string_view utf8) {
    fx_layout* layout = find_layout(utf8);
    return layout ? layout->advance : 0.0f;
}

float grapheme_shaper::measure_glyph(std::u32string_view utf32) {
    fx_layout* layout = find_layout(utf32);
    return layout ? layout->advance : 0.0f;
}

float grapheme_shaper::measure_string(std::string_view utf8) {
    float width = 0.0f;
    for (size_t start = 0; start < utf8.size();) {
        char32_t codepoint = 0;
        const size_t codepoint_end = utf8_decoder::next_codepoint(utf8, start, &codepoint);
        const size_t cluster_end =
            utf8_decoder::next_grapheme_boundary(utf8, start, codepoint_end, codepoint);
        if (cluster_end <= start) {
            break;
        }
        width += codepoint_end == cluster_end
                     ? measure_glyph(codepoint)
                     : measure_glyph(utf8.substr(start, cluster_end - start));
        start = cluster_end;
    }
    return width;
}

float grapheme_shaper::measure_string(std::u32string_view utf32) {
    float width = 0.0f;
    for (size_t start = 0; start < utf32.size();) {
        char32_t codepoint = 0;
        const size_t codepoint_end = utf32_decoder::next_codepoint(utf32, start, &codepoint);
        const size_t cluster_end =
            utf32_decoder::next_grapheme_boundary(utf32, start, codepoint_end, codepoint);
        if (cluster_end <= start) {
            break;
        }
        width += codepoint_end == cluster_end
                     ? measure_glyph(codepoint)
                     : measure_glyph(utf32.substr(start, cluster_end - start));
        start = cluster_end;
    }
    return width;
}

template <typename Text, typename Decoder, typename Callback>
void grapheme_shaper::draw_string_impl(Callback& callback,
                                       double x,
                                       double y,
                                       color ordinary_color,
                                       Text text,
                                       bool draw_spaces,
                                       color space_color) {
    if (!font_ || !font_->font || text.empty()) {
        return;
    }

    double batch_origin = x;
    fx_layout batch;
    batch.line_height = font_->font->metrics().line_height;
    color batch_color = ordinary_color;

    const auto flush_batch = [&] {
        if (batch.glyphs.empty()) {
            return;
        }
        const float advance = batch.advance;
        callback({batch_origin, y}, batch_color, &batch);
        batch_origin += static_cast<double>(advance);
        batch = {};
        batch.line_height = font_->font->metrics().line_height;
    };

    for (size_t start = 0; start < text.size();) {
        char32_t base_cp = 0;
        const size_t base_end = Decoder::next_codepoint(text, start, &base_cp);
        size_t cluster_end = Decoder::next_grapheme_boundary(text, start, base_end, base_cp);
        if (cluster_end <= start) {
            break;
        }

        if (monospace_ && base_end == cluster_end &&
            is_ascii_operator(static_cast<base::Unichar>(base_cp))) {
            while (cluster_end < text.size()) {
                char32_t next_cp = 0;
                const size_t next_end = Decoder::next_codepoint(text, cluster_end, &next_cp);
                if (!is_ascii_operator(static_cast<base::Unichar>(next_cp))) {
                    break;
                }
                cluster_end = next_end;
            }
        }

        const bool single_codepoint = base_end == cluster_end;
        const Text source = text.substr(start, cluster_end - start);
        const bool space_marker =
            Decoder::supports_space_markers && draw_spaces && single_codepoint && base_cp == U' ';
        const color cluster_color = space_marker ? space_color : ordinary_color;
        fx_layout* cluster = space_marker       ? find_layout(U'\u00b7')
                             : single_codepoint ? find_layout(base_cp)
                                                : find_layout(source);
        if (cluster) {
            std::vector<fx_glyph> glyphs = cluster->glyphs;
            const float cluster_advance = cluster->advance;
            for (fx_glyph& glyph : glyphs) {
                const size_t source_offset =
                    single_codepoint ? start
                                     : Decoder::source_cluster(source, start, glyph.cluster);
                glyph.cluster = base::checked_cast<uint32_t>(source_offset);
            }

            if (!batch.glyphs.empty() && batch_color != cluster_color) {
                flush_batch();
            }
            if (batch.glyphs.size() + glyphs.size() > kMaximumBatchGlyphs) {
                flush_batch();
            }
            if (glyphs.size() >= kMaximumBatchGlyphs) {
                fx_layout large = *cluster;
                large.glyphs = std::move(glyphs);
                large.advance = cluster_advance;
                callback({batch_origin, y}, cluster_color, &large);
                batch_origin += static_cast<double>(cluster_advance);
            } else {
                if (batch.glyphs.empty()) {
                    batch_color = cluster_color;
                }
                batch.glyphs.reserve(batch.glyphs.size() + glyphs.size());
                for (fx_glyph glyph : glyphs) {
                    glyph.x_offset += batch.advance;
                    batch.glyphs.push_back(glyph);
                }
                batch.advance += cluster_advance;
            }
        }
        start = cluster_end;
    }

    flush_batch();
}

void grapheme_shaper::draw_string(px_render_context* context,
                                  double x,
                                  double y,
                                  color value,
                                  std::string_view utf8,
                                  bool draw_spaces,
                                  color space_color,
                                  bool fade,
                                  float fade_start,
                                  float fade_end) {
    if (!context) {
        return;
    }
    auto callback = [&](vec2 position, color emitted_color, fx_layout* layout) {
        if (fade) {
            context->draw_shaped_text_faded(font_, position, emitted_color, layout, false,
                                            fade_start, fade_end);
        } else {
            context->draw_shaped_text(font_, position, emitted_color, layout, false);
        }
    };
    draw_string_impl<std::string_view, utf8_decoder>(callback, x, y, value, utf8, draw_spaces,
                                                     space_color);
}

void grapheme_shaper::draw_string(px_render_context* context,
                                  double x,
                                  double y,
                                  color value,
                                  std::u32string_view utf32,
                                  bool draw_spaces,
                                  color space_color,
                                  bool clip,
                                  float clip_start,
                                  float clip_end) {
    if (!context) {
        return;
    }
    auto callback = [&](vec2 position, color emitted_color, fx_layout* layout) {
        if (clip) {
            context->draw_shaped_text_clipped(font_, position, emitted_color, layout, false,
                                              clip_start, clip_end);
        } else {
            context->draw_shaped_text(font_, position, emitted_color, layout, false);
        }
    };
    draw_string_impl<std::u32string_view, utf32_decoder>(callback, x, y, value, utf32, draw_spaces,
                                                         space_color);
}

grapheme_shaper* grapheme_shaper::instance(px_font_t* font) {
    if (!font) {
        return nullptr;
    }
    auto& shaper = s_cache[font];
    if (!shaper) {
        shaper = std::make_unique<grapheme_shaper>(font, 0);
    }
    return shaper.get();
}
