#include "experiments/platform/fx/fx.h"

#include "base/unicode/unicode.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <uni_algo/prop.h>

namespace {

std::string utf16_to_utf8(std::u16string_view input) {
    std::string output;
    output.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
        uint32_t cp = input[i];
        if (cp >= 0xd800 && cp <= 0xdbff && i + 1 < input.size()) {
            const uint32_t low = input[i + 1];
            if (low >= 0xdc00 && low <= 0xdfff) {
                cp = 0x10000 + ((cp - 0xd800) << 10) + (low - 0xdc00);
                ++i;
            }
        }
        if (cp <= 0x7f) {
            output.push_back(static_cast<char>(cp));
        } else if (cp <= 0x7ff) {
            output.push_back(static_cast<char>(0xc0 | (cp >> 6)));
            output.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
        } else if (cp <= 0xffff) {
            output.push_back(static_cast<char>(0xe0 | (cp >> 12)));
            output.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3f)));
            output.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
        } else {
            output.push_back(static_cast<char>(0xf0 | (cp >> 18)));
            output.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3f)));
            output.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3f)));
            output.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
        }
    }
    return output;
}

bool is_ascii_operator(base::Unichar cp) {
    return (cp >= 0x21 && cp <= 0x2f) || (cp >= 0x3a && cp <= 0x40) ||
           (cp >= 0x5b && cp <= 0x60) || (cp >= 0x7b && cp <= 0x7e);
}

bool is_mark(base::Unichar cp) {
    if (cp < 0) {
        return false;
    }
    const una::codepoint::prop property{static_cast<char32_t>(cp)};
    return property.General_Category_Mn() || property.General_Category_Mc() ||
           property.General_Category_Me();
}

bool is_sublime_extend(base::Unichar cp) {
    return is_mark(cp) || (cp >= 0x1f3fb && cp <= 0x1f3ff) || (cp >= 0xfe00 && cp <= 0xfe0f);
}

// This is the smaller-than-UAX-29 boundary loop recovered from grapheme_shaper. In particular,
// Sublime does not merge Hangul jamo or Indic conjuncts here.
size_t sublime_grapheme_boundary(std::string_view text) {
    if (text.empty()) {
        return 0;
    }

    const auto is_regional_indicator = [](base::Unichar cp) {
        return cp >= 0x1f1e6 && cp <= 0x1f1ff;
    };

    size_t offset = 0;
    bool regional_indicator_armed = is_regional_indicator(base::next_utf8(text, offset));
    while (offset < text.size()) {
        size_t next = offset;
        const base::Unichar cp = base::next_utf8(text, next);
        if (regional_indicator_armed && is_regional_indicator(cp)) {
            offset = next;
            regional_indicator_armed = false;
            continue;
        }
        regional_indicator_armed = false;

        if (is_sublime_extend(cp)) {
            offset = next;
            continue;
        }
        if (cp == 0x200d) {
            offset = next;
            if (offset < text.size()) {
                base::next_utf8(text, offset);
            }
            continue;
        }
        break;
    }
    return offset;
}

}  // namespace

std::unique_ptr<fx_layout> fx_font::shape(std::u16string_view utf16) {
    return shape(utf16_to_utf8(utf16));
}

fx_font_widths fx_font::widths() {
    if (!widths_valid_) {
        const std::unique_ptr<fx_layout> em = shape("M");
        const std::unique_ptr<fx_layout> narrow = shape("i");
        widths_.em_width = em ? em->advance : 0.0f;
        widths_.monospace = em && narrow && std::abs(em->advance - narrow->advance) < 0.001f;
        widths_valid_ = true;
    }
    return widths_;
}

std::vector<fx_layout_batch> fx_shape_graphemes(fx_font* font,
                                                std::string_view utf8,
                                                bool snap_monospace_advances) {
    if (!font) {
        return {};
    }

    constexpr size_t kMaximumBatchGlyphs = 32;
    std::vector<fx_layout_batch> result;
    double batch_origin = 0.0;
    fx_layout batch;
    batch.line_height = font->metrics().line_height;
    const bool monospace = font->widths().monospace;
    const bool snap = monospace && snap_monospace_advances;

    const auto flush_batch = [&] {
        if (batch.glyphs.empty()) {
            return;
        }
        const float advance = batch.advance;
        result.push_back({.x_offset = batch_origin, .layout = std::move(batch)});
        batch_origin += static_cast<double>(advance);
        batch = {};
        batch.line_height = font->metrics().line_height;
    };

    for (size_t start = 0; start < utf8.size();) {
        size_t cluster_end = start + sublime_grapheme_boundary(utf8.substr(start));
        if (cluster_end <= start) {
            break;
        }

        size_t base_end = start;
        const base::Unichar base_cp = base::next_utf8(utf8, base_end);
        if (monospace && base_end == cluster_end && is_ascii_operator(base_cp)) {
            while (cluster_end < utf8.size()) {
                size_t next = cluster_end;
                if (!is_ascii_operator(base::next_utf8(utf8, next))) {
                    break;
                }
                cluster_end = next;
            }
        }

        std::unique_ptr<fx_layout> cluster = font->shape(utf8.substr(start, cluster_end - start));
        if (cluster) {
            if (snap) {
                float advance_delta = 0.0f;
                float cluster_advance = 0.0f;
                for (fx_glyph& glyph : cluster->glyphs) {
                    const float advance = std::round(glyph.advance);
                    advance_delta += advance - glyph.advance;
                    glyph.advance = advance;
                    glyph.x_offset += advance_delta;
                    cluster_advance += advance;
                }
                cluster->advance = cluster_advance;
            }
            for (fx_glyph& glyph : cluster->glyphs) {
                glyph.cluster += start;
            }

            if (batch.glyphs.size() + cluster->glyphs.size() > kMaximumBatchGlyphs) {
                flush_batch();
            }
            if (cluster->glyphs.size() >= kMaximumBatchGlyphs) {
                const float advance = cluster->advance;
                result.push_back({.x_offset = batch_origin, .layout = std::move(*cluster)});
                batch_origin += static_cast<double>(advance);
            } else {
                if (batch.glyphs.empty()) {
                    batch.primary_y_offset = cluster->primary_y_offset;
                    batch.primary_face_ascent = cluster->primary_face_ascent;
                }
                batch.glyphs.reserve(batch.glyphs.size() + cluster->glyphs.size());
                for (const fx_glyph& glyph : cluster->glyphs) {
                    fx_glyph positioned = glyph;
                    positioned.x_offset = batch.advance + glyph.x_offset;
                    batch.glyphs.push_back(positioned);
                }
                batch.advance += cluster->advance;
            }
        }
        start = cluster_end;
    }

    flush_batch();
    return result;
}

fx_glyph_cache::fx_glyph_cache(fx_font* font, float scale)
    : font_(font), scale_(scale > 0.0f ? scale : 1.0f) {}

uint64_t fx_glyph_cache::key(uint32_t glyph, unsigned phase) {
    return (static_cast<uint64_t>(phase) << 32) | glyph;
}

const fx_glyph_cache::glyph_data& fx_glyph_cache::lookup_glyph_data(uint32_t glyph,
                                                                    unsigned phase,
                                                                    bool alternate) {
    auto& cache = alternate ? alternate_ : normal_;
    const uint64_t cache_key = key(glyph, phase);
    auto found = cache.find(cache_key);
    if (found != cache.end()) {
        return found->second;
    }

    glyph_data data;
    if (font_) {
        const float subpixel_x = static_cast<float>(phase % 6) * (1.0f / 6.0f) * scale_;
        data.bitmap = font_->rasterise(glyph, scale_, subpixel_x);
        if (!data.bitmap.colored) {
            if (const fx_gamma_ramp* ramp = font_->gamma_ramp()) {
                for (size_t i = 0; i + 3 < data.bitmap.pixels.size(); i += 4) {
                    data.bitmap.pixels[i] = ramp->values[data.bitmap.pixels[i]];
                    data.bitmap.pixels[i + 1] = ramp->values[data.bitmap.pixels[i + 1]];
                    data.bitmap.pixels[i + 2] = ramp->values[data.bitmap.pixels[i + 2]];
                    if (ramp->complement_inverse && !alternate) {
                        data.bitmap.pixels[i] =
                            0xFF ^ ramp->inverse_values[0xFF ^ data.bitmap.pixels[i]];
                        data.bitmap.pixels[i + 1] =
                            0xFF ^ ramp->inverse_values[0xFF ^ data.bitmap.pixels[i + 1]];
                        data.bitmap.pixels[i + 2] =
                            0xFF ^ ramp->inverse_values[0xFF ^ data.bitmap.pixels[i + 2]];
                    }
                    if (alternate) {
                        data.bitmap.pixels[i] ^= 0xFF;
                        data.bitmap.pixels[i + 1] ^= 0xFF;
                        data.bitmap.pixels[i + 2] ^= 0xFF;
                    }
                }
            }
        }
    }
    return cache.emplace(cache_key, std::move(data)).first->second;
}

void fx_apply_font_glow(fx_glyph_bitmap* bitmap, float radius, bool preserve_source) {
    if (!bitmap || bitmap->empty()) {
        return;
    }
    const int r = static_cast<int>(std::floor(radius));
    if (r < 2) {
        return;
    }

    const size_t width = bitmap->width;
    const size_t height = bitmap->height;
    const std::vector<uint8_t> source = bitmap->pixels;
    std::vector<float> weights(static_cast<size_t>(r * 2 + 1));
    const float sigma = std::max(0.5f, radius * 0.5f);
    float weight_sum = 0.0f;
    for (int i = -r; i <= r; ++i) {
        const float x = static_cast<float>(i);
        const float weight = std::exp(-(x * x) / (2.0f * sigma * sigma));
        weights[static_cast<size_t>(i + r)] = weight;
        weight_sum += weight;
    }
    for (float& weight : weights) {
        weight /= weight_sum;
    }

    // The binary builds a one-dimensional float kernel, performs horizontal and vertical passes,
    // and renormalizes at clipped edges. Do the same over all premultiplied channels.
    std::vector<float> horizontal(source.size(), 0.0f);
    std::vector<uint8_t> blurred(source.size(), 0);
    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            float used_weight = 0.0f;
            float channels[4] = {};
            for (int offset = -r; offset <= r; ++offset) {
                const int sx = static_cast<int>(x) + offset;
                if (sx < 0 || sx >= static_cast<int>(width)) {
                    continue;
                }
                const float weight = weights[static_cast<size_t>(offset + r)];
                const size_t source_offset = (y * width + static_cast<size_t>(sx)) * 4;
                for (size_t channel = 0; channel < 4; ++channel) {
                    channels[channel] += source[source_offset + channel] * weight;
                }
                used_weight += weight;
            }
            const size_t destination = (y * width + x) * 4;
            for (size_t channel = 0; channel < 4; ++channel) {
                horizontal[destination + channel] = channels[channel] / used_weight;
            }
        }
    }

    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            float used_weight = 0.0f;
            float channels[4] = {};
            for (int offset = -r; offset <= r; ++offset) {
                const int sy = static_cast<int>(y) + offset;
                if (sy < 0 || sy >= static_cast<int>(height)) {
                    continue;
                }
                const float weight = weights[static_cast<size_t>(offset + r)];
                const size_t source_offset = (static_cast<size_t>(sy) * width + x) * 4;
                for (size_t channel = 0; channel < 4; ++channel) {
                    channels[channel] += horizontal[source_offset + channel] * weight;
                }
                used_weight += weight;
            }
            const size_t destination = (y * width + x) * 4;
            for (size_t channel = 0; channel < 4; ++channel) {
                blurred[destination + channel] = static_cast<uint8_t>(
                    std::clamp(std::round(channels[channel] / used_weight), 0.0f, 255.0f));
            }
        }
    }

    if (preserve_source) {
        for (size_t i = 0; i < blurred.size(); ++i) {
            blurred[i] = std::max(blurred[i], source[i]);
        }
    }
    bitmap->pixels = std::move(blurred);
    bitmap->colored = false;
}
