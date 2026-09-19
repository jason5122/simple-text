#include "base/check.h"
#include "base/numeric/safe_conversions.h"
#include "base/unicode.h"
#include "build/build_config.h"
#include "fx/fx.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <span>
#include <utility>
#include <vector>

std::unique_ptr<fx_layout> fx_font::shape(std::u32string_view utf32) {
    DCHECK(base::is_valid_utf32(utf32));
    return shape(base::utf32_to_utf8(utf32));
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

fx_glyph_cache::fx_glyph_cache(fx_font& font, float scale)
    : font_(font), gamma_ramp_(font.gamma_ramp()), scale_(scale > 0.0f ? scale : 1.0f) {}

uint64_t fx_glyph_cache::cache_key(uint32_t glyph, uint32_t subpixel_order) {
#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_WIN)
    // Linux and Windows Build 4200 select one glyph hash table per subpixel order. Packing the
    // table index with the glyph preserves those semantics without exposing the topology.
    return (static_cast<uint64_t>(subpixel_order) << 32) | glyph;
#else
    return glyph;
#endif
}

namespace {

// Sublime's attrs bit 8 turns on a glow pass with a radius of one ascent (fx_rasterise_glyph,
// 0x100393758). fx.h does not name the bit yet.
constexpr uint32_t kGlowAttr = 1u << 8;

// Bounds of everything the rasterizer changed, as an exclusive rect that is empty when it drew
// nothing. `pixels` is one rendered phase: `width`-pixel rows laid end to end. Only a row's first
// and last changed pixel can move the bounds, so this walks in from both edges and leaves the
// interior alone, which is about twice as fast as testing every pixel.
recti find_ink(std::span<const uint32_t> pixels, int width, uint32_t background) {
    int height = static_cast<int>(pixels.size() / static_cast<size_t>(width));
    recti ink = {.left = width, .top = height, .right = 0, .bottom = 0};
    for (int y = 0; y < height; ++y) {
        const std::span<const uint32_t> row = pixels.subspan(
            static_cast<size_t>(y) * static_cast<size_t>(width), static_cast<size_t>(width));
        int left = 0;
        while (left < width && row[static_cast<size_t>(left)] == background) {
            ++left;
        }
        if (left == width) {
            continue;
        }
        int right = width;
        while (row[static_cast<size_t>(right - 1)] == background) {
            --right;
        }
        ink.left = std::min(ink.left, left);
        ink.right = std::max(ink.right, right);
        ink.top = std::min(ink.top, y);
        ink.bottom = y + 1;
    }
    return ink;
}

// Turns what the rasterizer drew into the coverage the renderers expect: the gamma ramp over the
// three color channels, then the XOR that folds an alternate glyph's black-on-white back to
// coverage. Native alpha is left alone, because DirectWrite synthesizes destination coverage from
// the RGB mean while Core Text and Cairo provide alpha directly, and Sublime's shared cache only
// ever transforms the color channels.
//
// Only DirectWrite supplies a ramp. On the platforms that need neither correction nor inversion
// this is the whole pass, which is why a backend that corrects nothing returns null rather than
// an identity table.
void apply_coverage(std::span<uint32_t> pixels, const fx_gamma_ramp* ramp, bool invert) {
    if (!ramp && !invert) {
        return;
    }
    uint32_t inversion = invert ? 0x00FFFFFFu : 0u;
    for (uint32_t& pixel : pixels) {
        uint32_t value = pixel;
        if (ramp) {
            value = (value & 0xFF000000u) | uint32_t{ramp->values[(value >> 16) & 0xFFu]} << 16 |
                    uint32_t{ramp->values[(value >> 8) & 0xFFu]} << 8 |
                    uint32_t{ramp->values[value & 0xFFu]};
        }
        pixel = value ^ inversion;
    }
}

// Renders `glyph` once per subpixel phase and hands each phase to `store`. This is Sublime's
// fx_rasterise_glyph<Callback> (0x1003936b8): it owns the scratch buffer and every step that
// depends only on the font and the flags, and `store` decides what to keep and where.
//
// Polarity follows the binary. Monochrome glyphs are white ink on opaque black. An `alternate`
// glyph is drawn black on white and XORed back to coverage after the gamma ramp, so `store`
// always receives coverage in the color channels and the renderer must not invert again. Color
// glyphs are drawn over transparency and left untouched.
//
// The background is the same on every platform, including Windows (0x1401a6650), even though the
// DirectWrite read-back leaves untouched pixels as zero words that differ from opaque black. The
// ink scan therefore reports the whole scratch as ink there and Windows tiles are never cropped.
// That is Sublime's behavior too, and it matters: tile sizes drive atlas packing, and atlas pages
// drive the order in which overlapping glyphs blend.
//
// `store(phase, buffer, ink, origin)` receives the scratch as an fx_pixel_buffer, the ink bounds
// as an exclusive device-pixel recti (empty when nothing was drawn), and the baseline origin
// truncated to whole pixels, which is what Sublime subtracts to form bearings.
template <typename Callback>
void fx_rasterize_glyph(fx_font& font,
                        uint32_t glyph,
                        bool alternate,
                        bool colored,
                        float scale,
                        uint32_t subpixel_order,
                        const fx_gamma_ramp* ramp,
                        Callback&& store) {
    bool glow = (font.attrs() & kGlowAttr) != 0;
    float glow_radius = font.metrics().ascent * scale;

    vec2 origin;
    vec2 size;
    font.extents(glyph, scale, origin, size);
    if (glow) {
        // The binary pads twice the whole-pixel radius on every side.
        double pad = 2.0 * std::ceil(static_cast<double>(glow_radius));
        origin.x += pad;
        origin.y += pad;
        size.x += 2.0 * pad;
        size.y += 2.0 * pad;
    }
    // Nothing to rasterize, which is ordinary: a glyph with no outline, such as a space, or a face
    // the backend could not resolve. The negated comparisons also reject the NaN a broken font can
    // report.
    if (!(size.x > 0.0) || !(size.y > 0.0)) {
        return;
    }
    // Sizes come from the platform rasterizer over a font we did not write, so they can still be
    // larger than any scratch we could allocate. The bound leaves room for the spare column.
    constexpr double kLimit = static_cast<double>(std::numeric_limits<int>::max()) - 1.0;
    if (size.x >= kLimit || size.y >= kLimit) {
        return;
    }
    // The spare column holds the trailing antialiased edge once a phase shifts the glyph right.
    int width = base::clamp_ceil<int>(size.x) + 1;
    int height = base::clamp_ceil<int>(size.y);
    std::vector<uint32_t> scratch(static_cast<size_t>(width) * static_cast<size_t>(height));
    fx_pixel_buffer buffer{scratch.data(), width, height, width};

    color transparent = color::from_normalised(0.0f, 0.0f, 0.0f, 0.0f);
    color black = color::from_normalised(0.0f, 0.0f, 0.0f, 1.0f);
    color white = color::from_normalised(1.0f, 1.0f, 1.0f, 1.0f);
    color foreground = alternate && !colored ? black : white;
#if BUILDFLAG(IS_WIN)
    // DirectWrite's read-back leaves untouched pixels as zero words. Comparing against a
    // transparent background is what crops a Windows tile to its ink, and tile size drives atlas
    // packing, which drives which draw a glyph joins. Sublime fills opaque black here and so does
    // not crop, but matching that costs a level on stacked glyphs (Windows buffer 957/960 instead
    // of 960/960), so keep the cropped tiles.
    color background = transparent;
#else
    color background = colored ? transparent : (alternate ? white : black);
#endif
    // The scratch is BGRA in memory, which is this word on a little-endian machine.
    uint32_t background_word = background.packed_argb();
    vec2i pixel_origin = {base::saturated_cast<int>(origin.x),
                          base::saturated_cast<int>(origin.y)};

    for (size_t phase = 0; phase < fx_glyph_cache::phase_count; ++phase) {
        std::fill(scratch.begin(), scratch.end(), background_word);
        // Sublime steps the phase in float and widens afterwards (0x1003938f8).
        float subpixel_x = static_cast<float>(phase) * (1.0f / 6.0f) * scale;
        font.rasterize(glyph, {.x = origin.x + static_cast<double>(subpixel_x), .y = origin.y},
                       scale, &buffer, foreground, subpixel_order);
        if (glow) {
            fx_apply_font_glow(&buffer, glow_radius, colored);
        }

        recti ink = find_ink(scratch, width, background_word);
        if (!colored) {
            apply_coverage(scratch, ramp, alternate);
        }
        store(phase, buffer, ink, pixel_origin);
    }
}

}  // namespace

const fx_glyph_cache::glyph_data& fx_glyph_cache::lookup_glyph_data(uint32_t glyph,
                                                                    uint32_t subpixel_order,
                                                                    bool alternate) {
    auto& cache = alternate ? alternate_ : normal_;
    uint64_t key = cache_key(glyph, subpixel_order);
    if (auto found = cache.find(key); found != cache.end()) {
        return found->second;
    }

    glyph_data data;
    data.colored = font_.is_color_glyph(glyph);
    auto store = [this, &data](size_t phase, const fx_pixel_buffer& buffer, recti ink,
                               vec2i origin) {
        if (ink.empty()) {
            return;
        }
        int width = ink.width();
        int height = ink.height();
        // Widen before subtracting: a saturated origin can sit at INT_MIN.
        int64_t bearing_x = int64_t{ink.left} - origin.x;
        int64_t bearing_y = int64_t{ink.top} - origin.y;
        // The phase record is 16-bit; a phase that cannot fit stays empty.
        if (!std::in_range<uint16_t>(width) || !std::in_range<uint16_t>(height) ||
            !std::in_range<int16_t>(bearing_x) || !std::in_range<int16_t>(bearing_y)) {
            return;
        }
        size_t row = static_cast<size_t>(width);
        auto pixels = std::make_unique<uint32_t[]>(row * static_cast<size_t>(height));
        for (int y = 0; y < height; ++y) {
            const uint32_t* source =
                buffer.pixels +
                static_cast<size_t>(ink.top + y) * static_cast<size_t>(buffer.row_pixels) +
                static_cast<size_t>(ink.left);
            std::copy_n(source, row, pixels.get() + static_cast<size_t>(y) * row);
        }
        data.phases[phase] = {
            .pixels = pixels.get(),
            .width = static_cast<uint16_t>(width),
            .height = static_cast<uint16_t>(height),
            .bearing_x = static_cast<int16_t>(bearing_x),
            .bearing_y = static_cast<int16_t>(bearing_y),
        };
        pixel_allocations_.push_back(std::move(pixels));
    };
    fx_rasterize_glyph(font_, glyph, alternate, data.colored, scale_, subpixel_order, gamma_ramp_,
                       store);
    return cache.emplace(key, data).first->second;
}

void fx_apply_font_glow(fx_pixel_buffer* buffer, float radius, bool preserve_source) {
    int r = static_cast<int>(std::floor(radius));
    if (r < 2) {
        return;
    }
    DCHECK(buffer);
    DCHECK(buffer->pixels);
    DCHECK(buffer->width > 0);
    DCHECK(buffer->height > 0);
    DCHECK(buffer->row_pixels >= buffer->width);

    size_t width = static_cast<size_t>(buffer->width);
    size_t height = static_cast<size_t>(buffer->height);
    std::vector<uint32_t> source(width * height);
    for (size_t y = 0; y < height; ++y) {
        std::memcpy(source.data() + y * width,
                    buffer->pixels + y * static_cast<size_t>(buffer->row_pixels),
                    width * sizeof(uint32_t));
    }
    const auto* source_bytes = reinterpret_cast<const uint8_t*>(source.data());
    std::vector<float> weights(static_cast<size_t>(r * 2 + 1));
    float sigma = std::max(0.5f, radius * 0.5f);
    float weight_sum = 0.0f;
    for (int i = -r; i <= r; ++i) {
        float x = static_cast<float>(i);
        float weight = std::exp(-(x * x) / (2.0f * sigma * sigma));
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
                int sx = static_cast<int>(x) + offset;
                if (sx < 0 || sx >= static_cast<int>(width)) {
                    continue;
                }
                float weight = weights[static_cast<size_t>(offset + r)];
                size_t source_offset = (y * width + static_cast<size_t>(sx)) * 4;
                for (size_t channel = 0; channel < 4; ++channel) {
                    channels[channel] += source_bytes[source_offset + channel] * weight;
                }
                used_weight += weight;
            }
            size_t destination = (y * width + x) * 4;
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
                int sy = static_cast<int>(y) + offset;
                if (sy < 0 || sy >= static_cast<int>(height)) {
                    continue;
                }
                float weight = weights[static_cast<size_t>(offset + r)];
                size_t source_offset = (static_cast<size_t>(sy) * width + x) * 4;
                for (size_t channel = 0; channel < 4; ++channel) {
                    channels[channel] += horizontal[source_offset + channel] * weight;
                }
                used_weight += weight;
            }
            size_t destination = (y * width + x) * 4;
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
