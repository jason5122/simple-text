#include "fx/fx.h"

#include "base/check.h"
#include "build/build_config.h"

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
    : font_(font),
      gamma_ramp_(font ? font->gamma_ramp() : nullptr),
      scale_(scale > 0.0f ? scale : 1.0f) {}

uint64_t fx_glyph_cache::cache_key(uint32_t glyph, uint32_t subpixel_order) {
#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_WIN)
    // Linux and Windows Build 4200 select one glyph hash table per subpixel order. Packing the
    // table index with the glyph preserves those semantics without exposing the topology.
    return (static_cast<uint64_t>(subpixel_order) << 32) | glyph;
#else
    return glyph;
#endif
}

const fx_glyph_cache::glyph_data& fx_glyph_cache::lookup_glyph_data(uint32_t glyph,
                                                                    uint32_t subpixel_order,
                                                                    bool alternate) {
    auto& cache = alternate ? alternate_ : normal_;
    const uint64_t key = cache_key(glyph, subpixel_order);
    auto found = cache.find(key);
    if (found != cache.end()) {
        return found->second;
    }

    glyph_data data;
    if (font_) {
        data.colored = font_->is_color_glyph(glyph);
        const bool background_affects_rasterization = font_->bg_affects_rasterize();
        const bool native_alternate =
            !data.colored && alternate && background_affects_rasterization;
        const color transparent = color::from_normalised(0.0f, 0.0f, 0.0f, 0.0f);
        const color black = color::from_normalised(0.0f, 0.0f, 0.0f, 1.0f);
        const color white = color::from_normalised(1.0f, 1.0f, 1.0f, 1.0f);
#if BUILDFLAG(IS_WIN)
        // DirectWrite copies untouched pixels with zero alpha; keep its comparison background
        // transparent so those pixels remain outside the cropped glyph bounds.
        const color background = data.colored || !background_affects_rasterization
                                     ? transparent
                                     : (native_alternate ? white : black);
#else
        // Core Text and Cairo composite into the supplied bitmap, so monochrome glyphs need an
        // opaque background to preserve their coverage channels.
        const color background = data.colored ? transparent : (native_alternate ? white : black);
#endif
        const color foreground = native_alternate ? black : white;

        vec2 origin;
        vec2 size;
        font_->extents(glyph, scale_, origin, size);
        if (!std::isfinite(origin.x) || !std::isfinite(origin.y) || !std::isfinite(size.x) ||
            !std::isfinite(size.y) || size.x <= 0.0 || size.y <= 0.0 ||
            size.x >= static_cast<double>(std::numeric_limits<int>::max()) ||
            size.y >= static_cast<double>(std::numeric_limits<int>::max())) {
            return cache.emplace(key, data).first->second;
        }

        const double raster_width = std::ceil(size.x) + 1.0;
        const double raster_height = std::ceil(size.y);
        if (raster_width > static_cast<double>(std::numeric_limits<int>::max()) ||
            raster_height > static_cast<double>(std::numeric_limits<int>::max())) {
            return cache.emplace(key, data).first->second;
        }
        const int width = static_cast<int>(raster_width);
        const int height = static_cast<int>(raster_height);
        const size_t pixel_count = static_cast<size_t>(width) * static_cast<size_t>(height);
        std::vector<uint32_t> scratch(pixel_count);
        fx_pixel_buffer buffer{scratch.data(), width, height, width};

        for (size_t phase_index = 0; phase_index < data.phases.size(); ++phase_index) {
            auto* scratch_bytes = reinterpret_cast<uint8_t*>(scratch.data());
            for (size_t i = 0; i < pixel_count * 4; i += 4) {
                scratch_bytes[i] = background.blue();
                scratch_bytes[i + 1] = background.green();
                scratch_bytes[i + 2] = background.red();
                scratch_bytes[i + 3] = background.alpha();
            }

            const double subpixel_x = static_cast<double>(phase_index) * (1.0 / 6.0) * scale_;
            font_->rasterize(glyph, {.x = origin.x + subpixel_x, .y = origin.y}, scale_, &buffer,
                             foreground, subpixel_order);

            int left = width;
            int top = height;
            int right = 0;
            int bottom = 0;
            bool has_ink = false;
            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    const size_t offset =
                        (static_cast<size_t>(y) * static_cast<size_t>(buffer.row_pixels) +
                         static_cast<size_t>(x)) *
                        4;
                    const bool differs = scratch_bytes[offset] != background.blue() ||
                                         scratch_bytes[offset + 1] != background.green() ||
                                         scratch_bytes[offset + 2] != background.red() ||
                                         scratch_bytes[offset + 3] != background.alpha();
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
                continue;
            }

            if (!data.colored) {
                for (size_t i = 0; i < pixel_count * 4; i += 4) {
                    // Preserve native alpha. DirectWrite synthesizes destination coverage from the
                    // RGB mean while copying its DIB, whereas Core Text and Cairo provide alpha
                    // directly. Sublime's shared cache only transforms the three color channels.
                    if (gamma_ramp_) {
                        scratch_bytes[i] = gamma_ramp_->values[scratch_bytes[i]];
                        scratch_bytes[i + 1] = gamma_ramp_->values[scratch_bytes[i + 1]];
                        scratch_bytes[i + 2] = gamma_ramp_->values[scratch_bytes[i + 2]];
                        if (gamma_ramp_->complement_inverse && !alternate) {
                            scratch_bytes[i] =
                                0xFF ^ gamma_ramp_->inverse_values[0xFF ^ scratch_bytes[i]];
                            scratch_bytes[i + 1] =
                                0xFF ^ gamma_ramp_->inverse_values[0xFF ^ scratch_bytes[i + 1]];
                            scratch_bytes[i + 2] =
                                0xFF ^ gamma_ramp_->inverse_values[0xFF ^ scratch_bytes[i + 2]];
                        }
                    }
                    if (alternate && !native_alternate) {
                        scratch_bytes[i] ^= 0xFF;
                        scratch_bytes[i + 1] ^= 0xFF;
                        scratch_bytes[i + 2] ^= 0xFF;
                    }
                }
            }

            const int cropped_width = right - left + 1;
            const int cropped_height = bottom - top + 1;
            const int bearing_x = left - static_cast<int>(std::round(origin.x));
            const int bearing_y = top - static_cast<int>(std::round(origin.y));
            if (cropped_width > std::numeric_limits<uint16_t>::max() ||
                cropped_height > std::numeric_limits<uint16_t>::max() ||
                bearing_x < std::numeric_limits<int16_t>::min() ||
                bearing_x > std::numeric_limits<int16_t>::max() ||
                bearing_y < std::numeric_limits<int16_t>::min() ||
                bearing_y > std::numeric_limits<int16_t>::max()) {
                continue;
            }

            const size_t cropped_pixel_count =
                static_cast<size_t>(cropped_width) * static_cast<size_t>(cropped_height);
            auto pixels = std::make_unique<uint32_t[]>(cropped_pixel_count);
            for (int y = 0; y < cropped_height; ++y) {
                const uint32_t* source =
                    buffer.pixels +
                    static_cast<size_t>(top + y) * static_cast<size_t>(buffer.row_pixels) +
                    static_cast<size_t>(left);
                std::memcpy(pixels.get() +
                                static_cast<size_t>(y) * static_cast<size_t>(cropped_width),
                            source, static_cast<size_t>(cropped_width) * sizeof(uint32_t));
            }

            glyph_phase& phase = data.phases[phase_index];
            phase.pixels = pixels.get();
            phase.width = static_cast<uint16_t>(cropped_width);
            phase.height = static_cast<uint16_t>(cropped_height);
            phase.bearing_x = static_cast<int16_t>(bearing_x);
            phase.bearing_y = static_cast<int16_t>(bearing_y);
            pixel_allocations_.push_back(std::move(pixels));
        }
    }
    return cache.emplace(key, data).first->second;
}

void fx_apply_font_glow(fx_pixel_buffer* buffer, float radius, bool preserve_source) {
    const int r = static_cast<int>(std::floor(radius));
    if (r < 2) {
        return;
    }
    DCHECK(buffer);
    DCHECK(buffer->pixels);
    DCHECK(buffer->width > 0);
    DCHECK(buffer->height > 0);
    DCHECK(buffer->row_pixels >= buffer->width);

    const size_t width = static_cast<size_t>(buffer->width);
    const size_t height = static_cast<size_t>(buffer->height);
    std::vector<uint32_t> source(width * height);
    for (size_t y = 0; y < height; ++y) {
        std::memcpy(source.data() + y * width,
                    buffer->pixels + y * static_cast<size_t>(buffer->row_pixels),
                    width * sizeof(uint32_t));
    }
    const auto* source_bytes = reinterpret_cast<const uint8_t*>(source.data());
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
    std::vector<float> horizontal(source.size() * 4, 0.0f);
    std::vector<uint32_t> blurred(source.size());
    auto* blurred_bytes = reinterpret_cast<uint8_t*>(blurred.data());
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
                    channels[channel] += source_bytes[source_offset + channel] * weight;
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
                blurred_bytes[destination + channel] = static_cast<uint8_t>(
                    std::clamp(std::round(channels[channel] / used_weight), 0.0f, 255.0f));
            }
        }
    }

    if (preserve_source) {
        for (size_t i = 0; i < blurred.size() * 4; ++i) {
            blurred_bytes[i] = std::max(blurred_bytes[i], source_bytes[i]);
        }
    }
    for (size_t y = 0; y < height; ++y) {
        std::memcpy(buffer->pixels + y * static_cast<size_t>(buffer->row_pixels),
                    blurred.data() + y * width, width * sizeof(uint32_t));
    }
}
