#include "fx/fx.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
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
        auto [color_entry, inserted] = color_glyphs_.try_emplace(glyph, false);
        if (inserted) {
            color_entry->second = font_->is_color_glyph(glyph);
        }
        const bool colored = color_entry->second;
        const bool background_affects_rasterization = font_->bg_affects_rasterize();
        const bool native_alternate = !colored && alternate && background_affects_rasterization;
        const color transparent = color::from_normalised(0.0f, 0.0f, 0.0f, 0.0f);
        const color black = color::from_normalised(0.0f, 0.0f, 0.0f, 1.0f);
        const color white = color::from_normalised(1.0f, 1.0f, 1.0f, 1.0f);
        // Canonical colors keep monochrome cache entries reusable for every renderer tint.
        const color background = colored || !background_affects_rasterization
                                     ? transparent
                                     : (native_alternate ? white : black);
        const color foreground = native_alternate ? black : white;

        vec2 origin;
        vec2 size;
        font_->extents(glyph, scale_, origin, size);
        if (!std::isfinite(origin.x) || !std::isfinite(origin.y) || !std::isfinite(size.x) ||
            !std::isfinite(size.y) || size.x <= 0.0 || size.y <= 0.0 ||
            size.x >= static_cast<double>(std::numeric_limits<int>::max()) ||
            size.y >= static_cast<double>(std::numeric_limits<int>::max())) {
            return cache.emplace(cache_key, std::move(bitmap)).first->second;
        }

        const size_t width = static_cast<size_t>(std::ceil(size.x)) + 1;
        const size_t height = static_cast<size_t>(std::ceil(size.y));
        if (width > std::numeric_limits<size_t>::max() / 4 / height) {
            return cache.emplace(cache_key, std::move(bitmap)).first->second;
        }
        bitmap.width = width;
        bitmap.height = height;
        bitmap.colored = colored;
        bitmap.pixels.resize(width * height * 4);
        for (size_t i = 0; i < bitmap.pixels.size(); i += 4) {
            bitmap.pixels[i] = background.blue();
            bitmap.pixels[i + 1] = background.green();
            bitmap.pixels[i + 2] = background.red();
            bitmap.pixels[i + 3] = background.alpha();
        }

        const double subpixel_x = static_cast<double>(phase % 6) * (1.0 / 6.0) * scale_;
        font_->rasterize(glyph, {.x = origin.x + subpixel_x, .y = origin.y}, scale_, bitmap,
                         foreground);

        size_t left = width;
        size_t top = height;
        size_t right = 0;
        size_t bottom = 0;
        bool has_ink = false;
        for (size_t y = 0; y < height; ++y) {
            for (size_t x = 0; x < width; ++x) {
                const size_t offset = (y * width + x) * 4;
                const bool differs = bitmap.pixels[offset] != background.blue() ||
                                     bitmap.pixels[offset + 1] != background.green() ||
                                     bitmap.pixels[offset + 2] != background.red() ||
                                     bitmap.pixels[offset + 3] != background.alpha();
                if (!differs) {
                    continue;
                }
                has_ink = true;
                left = std::min(left, x);
                top = std::min(top, y);
                right = std::max(right, x);
                bottom = std::max(bottom, y);
            }
        }
        if (!has_ink) {
            bitmap = {};
            return cache.emplace(cache_key, std::move(bitmap)).first->second;
        }

        if (!colored) {
            const fx_gamma_ramp* ramp = font_->gamma_ramp();
            for (size_t i = 0; i + 3 < bitmap.pixels.size(); i += 4) {
                const unsigned mean = static_cast<unsigned>(bitmap.pixels[i]) +
                                      static_cast<unsigned>(bitmap.pixels[i + 1]) +
                                      static_cast<unsigned>(bitmap.pixels[i + 2]);
                bitmap.pixels[i + 3] =
                    static_cast<uint8_t>(native_alternate ? 255u - mean / 3u : mean / 3u);
                if (ramp) {
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
                }
                if (alternate && !native_alternate) {
                    bitmap.pixels[i] ^= 0xFF;
                    bitmap.pixels[i + 1] ^= 0xFF;
                    bitmap.pixels[i + 2] ^= 0xFF;
                }
            }
        }

        const size_t cropped_width = right - left + 1;
        const size_t cropped_height = bottom - top + 1;
        std::vector<uint8_t> cropped(cropped_width * cropped_height * 4);
        for (size_t y = 0; y < cropped_height; ++y) {
            const uint8_t* source = bitmap.pixels.data() + ((top + y) * width + left) * 4;
            std::memcpy(cropped.data() + y * cropped_width * 4, source, cropped_width * 4);
        }
        bitmap.width = cropped_width;
        bitmap.height = cropped_height;
        bitmap.bearing_x = static_cast<int>(left) - static_cast<int>(std::round(origin.x));
        bitmap.bearing_y = static_cast<int>(top) - static_cast<int>(std::round(origin.y));
        bitmap.pixels = std::move(cropped);
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
