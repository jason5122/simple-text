#include "fx/fx.h"

#include <algorithm>
#include <cmath>
#include <string>

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

fx_glyph_cache::fx_glyph_cache(fx_font* font, float scale)
    : font_(font), scale_(scale > 0.0f ? scale : 1.0f) {}

uint64_t fx_glyph_cache::key(uint32_t glyph, unsigned phase) {
    return (static_cast<uint64_t>(phase) << 32) | glyph;
}

const fx_glyph_bitmap& fx_glyph_cache::lookup_glyph_data(uint32_t glyph,
                                                         unsigned phase,
                                                         bool alternate) {
    auto& cache = alternate ? alternate_ : normal_;
    const uint64_t cache_key = key(glyph, phase);
    auto found = cache.find(cache_key);
    if (found != cache.end()) {
        return found->second;
    }

    fx_glyph_bitmap bitmap;
    if (font_) {
        const double subpixel_x = static_cast<double>(phase % 6) * (1.0 / 6.0) * scale_;
        bitmap = font_->rasterise(glyph, {.x = subpixel_x}, scale_);
        if (!bitmap.colored) {
            if (const fx_gamma_ramp* ramp = font_->gamma_ramp()) {
                for (size_t i = 0; i + 3 < bitmap.pixels.size(); i += 4) {
                    bitmap.pixels[i] = ramp->values[bitmap.pixels[i]];
                    bitmap.pixels[i + 1] = ramp->values[bitmap.pixels[i + 1]];
                    bitmap.pixels[i + 2] = ramp->values[bitmap.pixels[i + 2]];
                    if (ramp->complement_inverse && !alternate) {
                        bitmap.pixels[i] = 0xFF ^ ramp->inverse_values[0xFF ^ bitmap.pixels[i]];
                        bitmap.pixels[i + 1] =
                            0xFF ^ ramp->inverse_values[0xFF ^ bitmap.pixels[i + 1]];
                        bitmap.pixels[i + 2] =
                            0xFF ^ ramp->inverse_values[0xFF ^ bitmap.pixels[i + 2]];
                    }
                    if (alternate) {
                        bitmap.pixels[i] ^= 0xFF;
                        bitmap.pixels[i + 1] ^= 0xFF;
                        bitmap.pixels[i + 2] ^= 0xFF;
                    }
                }
            }
        }
    }
    return cache.emplace(cache_key, std::move(bitmap)).first->second;
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
