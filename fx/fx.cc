#include "base/check.h"
#include "base/numeric/safe_conversions.h"
#include "base/unicode.h"
#include "build/build_config.h"
#include "fx/fx.h"
#include "fx/fx_internal.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <span>
#include <spdlog/spdlog.h>
#include <utility>
#include <vector>

std::unique_ptr<fx_layout> fx_font::shape(std::u32string_view utf32) {
    DCHECK(base::is_valid_utf32(utf32));
    return shape(base::utf32_to_utf8(utf32));
}

fx_font_widths fx_font::widths() {
    if (!widths_valid_) {
        std::unique_ptr<fx_layout> em = shape("M");
        std::unique_ptr<fx_layout> narrow = shape("i");
        widths_.em_width = em ? em->advance : 0.0f;
        widths_.monospace = em && narrow && std::abs(em->advance - narrow->advance) < 0.001f;
        widths_valid_ = true;
    }
    return widths_;
}

fx_glyph_cache::fx_glyph_cache(fx_font& font, float scale)
    : font_(font), gamma_ramp_(font.gamma_ramp()), scale_(scale > 0.0f ? scale : 1.0f) {}

size_t fx_glyph_cache::table_index(uint32_t platform_value, bool alternate) {
    if constexpr (alternate_glyphs) {
        return alternate ? 1 : 0;
    } else {
        return platform_value;
    }
}

// Bounds of everything the rasterizer changed, as an exclusive rect that is empty when it drew
// nothing. Only a row's first and last changed pixel can move the bounds, so this walks in from
// both edges and leaves the interior alone, which is about twice as fast as testing every pixel.
recti find_ink(const fx_pixel_buffer& buffer, uint32_t background) {
    recti ink = {.left = buffer.width, .top = buffer.height, .right = 0, .bottom = 0};
    for (int y = 0; y < buffer.height; ++y) {
        std::span<const uint32_t> row{buffer.pixels + static_cast<size_t>(y) *
                                                          static_cast<size_t>(buffer.row_pixels),
                                      static_cast<size_t>(buffer.width)};
        int left = 0;
        while (left < buffer.width && row[static_cast<size_t>(left)] == background) {
            ++left;
        }
        if (left == buffer.width) {
            continue;
        }
        int right = buffer.width;
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

// Sublime's fx_apply_font_glow (0x1002abca8), which attrs bit 8 runs over each rendered phase
// before the ink scan. `colored` is the glyph's is_color_glyph result; the binary re-reads it at
// the call site rather than passing the cache's copy down.
//
// Two things keep the work proportional to the ink rather than to the padded scratch: the kernel
// is only `radius | 1` taps wide, and the blur covers the ink grown by half the radius, which is
// as far as those taps reach. Everything outside keeps the background the rasterizer filled in, so
// the padding the caller allocated is never all used.
void fx_apply_font_glow(fx_pixel_buffer* buffer, int radius, bool colored) {
    if (radius < 2) return;

    DCHECK(buffer);
    DCHECK(buffer->pixels);
    DCHECK(buffer->width > 0);
    DCHECK(buffer->height > 0);
    DCHECK(buffer->row_pixels >= buffer->width);

    // The rasterizer fills the whole scratch with one background word before drawing, so the
    // first pixel is that word, and ink is every pixel that no longer matches it.
    uint32_t background_word = buffer->pixels[0];
    recti ink = find_ink(*buffer, background_word);
    if (ink.empty()) {
        return;
    }
    int half = (radius | 1) / 2;
    recti region = {
        .left = std::max(ink.left - radius / 2, 0),
        .top = std::max(ink.top - radius / 2, 0),
        .right = std::min(ink.right + radius / 2, buffer->width),
        .bottom = std::min(ink.bottom + radius / 2, buffer->height),
    };
    size_t region_width = static_cast<size_t>(region.width());
    size_t region_height = static_cast<size_t>(region.height());

    auto pixel_at = [buffer](int x, int y) -> uint32_t* {
        return buffer->pixels + static_cast<size_t>(y) * static_cast<size_t>(buffer->row_pixels) +
               static_cast<size_t>(x);
    };
    // Straight, not premultiplied, 0..1 per channel: the same fcolor round trip the binary makes.
    //
    // A monochrome glyph can carry different coverage per channel from subpixel antialiasing, and
    // the binary flattens the three to their mean before blurring, which keeps the halo grey. Once
    // flattened those three are equal, and a separable blur is per-channel linear, so carrying all
    // three produces the same number three times. The mono path stores the mean and the alpha
    // only: half the working set and half the arithmetic, for what is by far the common case.
    size_t channels = colored ? 4 : 2;
    std::vector<float> source(region_width * region_height * channels);
    for (size_t y = 0; y < region_height; ++y) {
        const uint32_t* row = pixel_at(region.left, region.top + static_cast<int>(y));
        for (size_t x = 0; x < region_width; ++x) {
            fcolor pixel = fcolor::from_packed_argb(row[x]);
            std::span<float> out =
                std::span(source).subspan((y * region_width + x) * channels, channels);
            if (colored) {
                out[0] = pixel.r;
                out[1] = pixel.g;
                out[2] = pixel.b;
                out[3] = pixel.a;
            } else {
                out[0] = (pixel.r + pixel.g + pixel.b) / 3.0f;
                out[1] = pixel.a;
            }
        }
    }

    // exp(-x*x/2) sampled over `taps` positions a quarter-kernel apart, so the standard deviation
    // is taps/4 and the window spans two of them either side. Centering on taps/2 while the taps
    // are indexed from taps/2 rounded down puts the peak half a pixel past the middle tap, and
    // the passes below then use every weight but the last.
    int taps = radius | 1;
    std::vector<float> weights(static_cast<size_t>(taps));
    float weight_sum = 0.0f;
    for (int i = 0; i < taps; ++i) {
        float x = (static_cast<float>(i) - static_cast<float>(taps) / 2.0f) /
                  (static_cast<float>(taps) / 4.0f);
        // Widened exactly where the binary widens it.
        double exponent = -0.5 * static_cast<double>(x) * static_cast<double>(x);
        weights[static_cast<size_t>(i)] = static_cast<float>(std::exp(exponent));
        weight_sum += weights[static_cast<size_t>(i)];
    }
    for (float& weight : weights) {
        weight /= weight_sum;
    }

    // Horizontal into `blurred`, vertical back into `source`. Both divide by the weight they
    // actually used, so a window clipped by the region edge does not darken the result.
    std::vector<float> blurred(source.size());
    for (size_t y = 0; y < region_height; ++y) {
        for (size_t x = 0; x < region_width; ++x) {
            size_t first = static_cast<size_t>(std::max(static_cast<int>(x) - half, 0));
            size_t last = std::min(x + static_cast<size_t>(half), region_width);  // exclusive
            std::array<float, 4> sum{};
            float used_weight = 0.0f;
            for (size_t sx = first; sx < last; ++sx) {
                float weight = weights[sx - x + static_cast<size_t>(half)];
                std::span<const float> pixel =
                    std::span(source).subspan((y * region_width + sx) * channels, channels);
                for (size_t c = 0; c < channels; ++c) {
                    sum[c] += pixel[c] * weight;
                }
                used_weight += weight;
            }
            std::span<float> out =
                std::span(blurred).subspan((y * region_width + x) * channels, channels);
            for (size_t c = 0; c < channels; ++c) {
                out[c] = sum[c] / used_weight;
            }
        }
    }
    for (size_t x = 0; x < region_width; ++x) {
        for (size_t y = 0; y < region_height; ++y) {
            size_t first = static_cast<size_t>(std::max(static_cast<int>(y) - half, 0));
            size_t last = std::min(y + static_cast<size_t>(half), region_height);  // exclusive
            std::array<float, 4> sum{};
            float used_weight = 0.0f;
            for (size_t sy = first; sy < last; ++sy) {
                float weight = weights[sy - y + static_cast<size_t>(half)];
                std::span<const float> pixel =
                    std::span(blurred).subspan((sy * region_width + x) * channels, channels);
                for (size_t c = 0; c < channels; ++c) {
                    sum[c] += pixel[c] * weight;
                }
                used_weight += weight;
            }
            std::span<float> out =
                std::span(source).subspan((y * region_width + x) * channels, channels);
            for (size_t c = 0; c < channels; ++c) {
                out[c] = sum[c] / used_weight;
            }
        }
    }

    // The glow is added to what the glyph already contributes over the background, not swapped in
    // for it, which is what keeps a glowing glyph's core opaque.
    fcolor background = fcolor::from_packed_argb(background_word);
    auto combine = [](float glow, float original, float base) {
        return std::clamp(glow + (original - base), 0.0f, 1.0f);
    };
    for (size_t y = 0; y < region_height; ++y) {
        uint32_t* row = pixel_at(region.left, region.top + static_cast<int>(y));
        for (size_t x = 0; x < region_width; ++x) {
            std::span<const float> blur =
                std::span(source).subspan((y * region_width + x) * channels, channels);
            // A monochrome halo is grey, so its single blurred plane feeds all three colour
            // channels; only the alpha is carried separately.
            fcolor glow = colored ? fcolor{blur[0], blur[1], blur[2], blur[3]}
                                  : fcolor{blur[0], blur[0], blur[0], blur[1]};
            fcolor original = fcolor::from_packed_argb(row[x]);
            row[x] = fcolor{combine(glow.r, original.r, background.r),
                            combine(glow.g, original.g, background.g),
                            combine(glow.b, original.b, background.b),
                            combine(glow.a, original.a, background.a)}
                         .packed_argb();
        }
    }
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

namespace {

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
                        uint32_t platform_value,
                        const fx_gamma_ramp* ramp,
                        Callback&& store) {
    bool glow = (font.attrs() & FX_FONT_GLOW) != 0;
    // The radius is the face's own ascent scaled to the device, so a malformed file can ask for a
    // halo thousands of pixels wide. Sublime has no cap because it only rasterizes faces the
    // platform resolved, while fx_create_font_from_file takes whatever file it is handed.
    //
    // std::min leaves a NaN ascent alone, which the size checks below then reject.
    float glow_radius = std::min(font.metrics().ascent * scale, fx_glyph_cache::kMaxGlowRadius);
    // The blur works in whole pixels while the padding below rounds the other way, so the two
    // roundings are spelled out here rather than hidden inside fx_apply_font_glow. The binary
    // truncates toward zero for the kernel; its unsigned convert saturates, so a negative or NaN
    // radius lands on zero and takes the early return there.
    int glow_radius_px = base::saturated_cast<int>(glow_radius);

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

    // Nothing to rasterize (e.g., a space), which is the zero case rather than the negative one.
    // The negated comparisons also reject the NaN a broken font can report.
    if (!(size.x > 0.0) || !(size.y > 0.0)) return;

    // The spare column holds the trailing antialiased edge once a phase shifts the glyph right.
    double columns = std::ceil(size.x) + 1.0;
    double rows = std::ceil(size.y);
    // A file can make a glyph thousands of em wide: 16 units per em is legal, and int16
    // coordinates then reach 2000 em, which at 20px is 40000 device pixels on a side and 6.4 GB of
    // scratch per phase, plus as much again per tile for whatever the ink scan keeps. Sublime
    // allocates whatever is asked. Checking in double before converting also keeps the int
    // conversions below exact.
    constexpr double kMaxSide = fx_glyph_cache::kMaxGlyphSide;
    constexpr double kMaxPixels = fx_glyph_cache::kMaxGlyphPixels;
    if (columns > kMaxSide || rows > kMaxSide) return;
    if (columns * rows > kMaxPixels) return;
    int width = static_cast<int>(columns);
    int height = static_cast<int>(rows);
    // Every phase refills the whole buffer with the background before drawing, so the
    // zero-initialization a std::vector would insist on is dead work.
    size_t pixel_count = static_cast<size_t>(width) * static_cast<size_t>(height);
    auto storage = std::make_unique_for_overwrite<uint32_t[]>(pixel_count);
    std::span<uint32_t> scratch{storage.get(), pixel_count};
    fx_pixel_buffer buffer{scratch.data(), width, height, width};

    color transparent = color::from_normalised(0.0f, 0.0f, 0.0f, 0.0f);
    color black = color::from_normalised(0.0f, 0.0f, 0.0f, 1.0f);
    color white = color::from_normalised(1.0f, 1.0f, 1.0f, 1.0f);
    color foreground = alternate && !colored ? black : white;
#if BUILDFLAG(IS_WIN)
    // DirectWrite's background pixels always have a value of 0.
    color background = transparent;
#else
    color background = colored ? transparent : (alternate ? white : black);
#endif
    // The scratch is BGRA in memory, which is this word on a little-endian machine.
    uint32_t background_word = background.packed_argb();
    vec2i pixel_origin = {base::saturated_cast<int>(origin.x),
                          base::saturated_cast<int>(origin.y)};

    for (size_t phase = 0; phase < fx_glyph_cache::phase_count; ++phase) {
        std::ranges::fill(scratch, background_word);
        // Sublime steps the phase in float and widens afterwards (0x1003938f8).
        float subpixel_x = static_cast<float>(phase) * (1.0f / 6.0f) * scale;
        font.rasterize(glyph, {.x = origin.x + static_cast<double>(subpixel_x), .y = origin.y},
                       scale, &buffer, foreground, platform_value);
        if (glow) {
            fx_apply_font_glow(&buffer, glow_radius_px, colored);
        }

        recti ink = find_ink(buffer, background_word);
        if (!colored) {
            apply_coverage(scratch, ramp, alternate);
        }
        store(phase, buffer, ink, pixel_origin);
    }
}

}  // namespace

const fx_glyph_cache::glyph_data& fx_glyph_cache::lookup_glyph_data(uint32_t glyph,
                                                                    uint32_t platform_value,
                                                                    bool alternate) {
    size_t index = table_index(platform_value, alternate);
    if (tables_.size() <= index) {
        tables_.resize(index + 1);
    }
    auto& table = tables_[index];
    if (auto found = table.find(glyph); found != table.end()) {
        return found->second;
    }

    glyph_data data;
    data.colored = font_.is_color_glyph(glyph);
    auto store = [this, &data](size_t phase, const fx_pixel_buffer& buffer, recti ink,
                               vec2i origin) {
        if (ink.empty()) return;

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
        size_t count = row * static_cast<size_t>(height);
        auto pixels = std::make_unique_for_overwrite<uint32_t[]>(count);
        // Every element is written below, which is what makes the uninitialized allocation safe.
        std::span<const uint32_t> scratch{buffer.pixels, static_cast<size_t>(buffer.row_pixels) *
                                                             static_cast<size_t>(buffer.height)};
        std::span<uint32_t> tile{pixels.get(), count};
        for (size_t y = 0; y < static_cast<size_t>(height); ++y) {
            size_t source =
                (static_cast<size_t>(ink.top) + y) * static_cast<size_t>(buffer.row_pixels) +
                static_cast<size_t>(ink.left);
            std::ranges::copy(scratch.subspan(source, row), tile.subspan(y * row, row).begin());
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
    fx_rasterize_glyph(font_, glyph, alternate, data.colored, scale_, platform_value, gamma_ramp_,
                       store);
    return table.emplace(glyph, data).first->second;
}

namespace {

// Whether the platform gave us the positive distances fx_font_metrics promises. hhea and OS/2 are
// signed, so a file can put the ascent below the baseline or make the line height zero, and a
// zero unitsPerEm turns every metric into NaN on a backend that does not refuse the file itself.
// The editor sizes rows with these numbers, so a font that fails here is not worth returning.
bool has_usable_metrics(const fx_font& font) {
    fx_font_metrics metrics = font.metrics();
    return std::isfinite(metrics.ascent) && std::isfinite(metrics.descent) &&
           std::isfinite(metrics.leading) && std::isfinite(metrics.line_height) &&
           std::isfinite(font.raster_ascent()) && metrics.ascent >= 0.0f &&
           metrics.descent >= 0.0f && metrics.line_height > 0.0f;
}

std::unique_ptr<fx_font> checked(std::unique_ptr<fx_font> font, std::string_view source) {
    if (font && !has_usable_metrics(*font)) {
        fx_font_metrics metrics = font->metrics();
        spdlog::error("font \"{}\" has unusable metrics: ascent {} descent {} line height {}",
                      source, metrics.ascent, metrics.descent, metrics.line_height);
        return nullptr;
    }
    return font;
}

bool usable_size(float size, std::string_view source) {
    // Core Text substitutes 12pt for a zero size and DirectWrite fails the text format; refusing
    // here keeps the backends' answers the same.
    if (size > 0.0f && std::isfinite(size)) return true;
    spdlog::error("font \"{}\" requested at an unusable size {}", source, size);
    return false;
}

}  // namespace

std::unique_ptr<fx_font> fx_create_font(std::string_view family, float size, uint32_t attrs) {
    if (!usable_size(size, family)) return nullptr;
    return checked(fx_backend_create_font(family, size, attrs), family);
}

std::unique_ptr<fx_font> fx_create_font_from_file(std::string_view path,
                                                  float size,
                                                  uint32_t attrs) {
    if (!usable_size(size, path)) return nullptr;
    return checked(fx_backend_create_font_from_file(path, size, attrs), path);
}
