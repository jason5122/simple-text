#include "experiments/platform/px/skia_render_context.h"

#include "experiments/platform/px/px_font_internal.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

#if defined(__ARM_NEON)
#include <arm_neon.h>
#endif

#include "include/core/SkCanvas.h"
#include "include/core/SkColor.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkPaint.h"
#include "include/core/SkRect.h"
#include "include/core/SkSurface.h"

namespace {

recti intersect_recti(recti a, recti b) {
    recti result{
        std::max(a.left, b.left),
        std::max(a.top, b.top),
        std::min(a.right, b.right),
        std::min(a.bottom, b.bottom),
    };
    if (result.empty()) {
        return {};
    }
    return result;
}

uint8_t multiply_bytes(uint8_t a, uint8_t b) {
    return static_cast<uint8_t>((static_cast<unsigned>(a) * b + 127u) / 255u);
}

uint8_t source_over_channel(uint8_t source, uint8_t destination, uint8_t source_alpha) {
    const unsigned value =
        static_cast<unsigned>(source) + multiply_bytes(destination, 255u - source_alpha);
    return static_cast<uint8_t>(std::min(value, 255u));
}

#if defined(__ARM_NEON)

uint8x8_t multiply_bytes(uint8x8_t a, uint8x8_t b) {
    const uint16x8_t product = vmull_u8(a, b);
    const uint16x8_t biased = vaddq_u16(product, vdupq_n_u16(128));
    return vshrn_n_u16(vaddq_u16(biased, vshrq_n_u16(biased, 8)), 8);
}

uint8x8_t source_over_channel(uint8x8_t source, uint8x8_t destination, uint8x8_t source_alpha) {
    return vqadd_u8(source, multiply_bytes(destination, vmvn_u8(source_alpha)));
}

#endif

void composite_glyph_scanline(uint8_t* destination,
                              const uint8_t* source,
                              int pixel_count,
                              const uint8_t tint[4],
                              bool colored,
                              bool alternate) {
    int x = 0;
#if defined(__ARM_NEON)
    const bool opaque_tint = tint[3] == 255u;
    const uint8x8_t tint_alpha = vdup_n_u8(tint[3]);
    for (; x + 8 <= pixel_count; x += 8, destination += 32, source += 32) {
        uint8x8x4_t src = vld4_u8(source);
        const uint8x8x4_t dst = vld4_u8(destination);
        uint8x8x4_t result;
        if (colored) {
            const uint8x8_t source_alpha =
                opaque_tint ? src.val[3] : multiply_bytes(src.val[3], tint_alpha);
            for (int channel = 0; channel < 3; ++channel) {
                const uint8x8_t source_channel =
                    opaque_tint ? src.val[channel] : multiply_bytes(src.val[channel], tint_alpha);
                result.val[channel] =
                    source_over_channel(source_channel, dst.val[channel], source_alpha);
            }
            result.val[3] = vminv_u8(dst.val[3]) == 255u
                                ? vdup_n_u8(255u)
                                : source_over_channel(source_alpha, dst.val[3], source_alpha);
        } else {
            for (int channel = 0; channel < 3; ++channel) {
                if (alternate) {
                    src.val[channel] = vmvn_u8(src.val[channel]);
                }
                const uint8x8_t coverage =
                    opaque_tint ? src.val[channel] : multiply_bytes(src.val[channel], tint_alpha);
                const uint8x8_t tinted = multiply_bytes(vdup_n_u8(tint[channel]), coverage);
                result.val[channel] = source_over_channel(tinted, dst.val[channel], coverage);
            }
            const uint8x8_t coverage =
                opaque_tint ? src.val[3] : multiply_bytes(src.val[3], tint_alpha);
            result.val[3] = vminv_u8(dst.val[3]) == 255u
                                ? vdup_n_u8(255u)
                                : source_over_channel(coverage, dst.val[3], coverage);
        }
        vst4_u8(destination, result);
    }
#endif

    for (; x < pixel_count; ++x, destination += 4, source += 4) {
        if (colored) {
            const uint8_t source_alpha = multiply_bytes(source[3], tint[3]);
            destination[0] = source_over_channel(multiply_bytes(source[0], tint[3]),
                                                 destination[0], source_alpha);
            destination[1] = source_over_channel(multiply_bytes(source[1], tint[3]),
                                                 destination[1], source_alpha);
            destination[2] = source_over_channel(multiply_bytes(source[2], tint[3]),
                                                 destination[2], source_alpha);
            destination[3] = source_over_channel(source_alpha, destination[3], source_alpha);
        } else {
            for (int channel = 0; channel < 3; ++channel) {
                const uint8_t glyph_coverage =
                    alternate ? static_cast<uint8_t>(source[channel] ^ 0xffu) : source[channel];
                const uint8_t coverage = multiply_bytes(glyph_coverage, tint[3]);
                const uint8_t tinted = multiply_bytes(tint[channel], coverage);
                destination[channel] = source_over_channel(tinted, destination[channel], coverage);
            }
            const uint8_t coverage = multiply_bytes(source[3], tint[3]);
            destination[3] = source_over_channel(coverage, destination[3], coverage);
        }
    }
}

}  // namespace

class skia_render_context::impl {
public:
    explicit impl(const px_pixel_buffer& buffer) {
        if (!buffer.pixels || buffer.width <= 0 || buffer.height <= 0 ||
            buffer.row_bytes < static_cast<size_t>(buffer.width) * 4u) {
            return;
        }
        // px_pixel_buffer is BGRA on every platform. On macOS Skia's native N32 format is RGBA,
        // while the Core Graphics image consuming this buffer is explicitly BGRA. Make the
        // format explicit so Skia does not exchange red and blue for geometry; the custom glyph
        // compositor below already writes BGRA.
        surface_ =
            SkSurfaces::WrapPixels(SkImageInfo::Make(buffer.width, buffer.height,
                                                     kBGRA_8888_SkColorType, kPremul_SkAlphaType),
                                   buffer.pixels, buffer.row_bytes);
    }

    SkCanvas* canvas() const { return surface_ ? surface_->getCanvas() : nullptr; }

    void pixels_will_change() {
        if (surface_ && !pixels_changed_) {
            surface_->notifyContentWillChange(SkSurface::kRetain_ContentChangeMode);
            pixels_changed_ = true;
        }
    }

private:
    sk_sp<SkSurface> surface_;
    bool pixels_changed_ = false;
};

skia_render_context::skia_render_context(px_pixel_buffer buffer, recti clip, double dpi_scale)
    : buffer_(buffer),
      clip_(intersect_recti(clip, recti{0, 0, buffer.width, buffer.height})),
      dpi_scale_(dpi_scale > 0.0 ? dpi_scale : 1.0),
      scale_{dpi_scale_, dpi_scale_},
      impl_(std::make_unique<impl>(buffer)) {
    if (SkCanvas* canvas = impl_->canvas(); canvas && !clip_.empty()) {
        canvas->clipRect(
            SkRect::MakeLTRB(static_cast<float>(clip_.left), static_cast<float>(clip_.top),
                             static_cast<float>(clip_.right), static_cast<float>(clip_.bottom)));
    }
}

skia_render_context::~skia_render_context() = default;

bool skia_render_context::valid() const { return impl_ && impl_->canvas() && !clip_.empty(); }

rect skia_render_context::transformed_rect(rect area) const {
    const double x0 = translation_.x + area.x * scale_.x;
    const double y0 = translation_.y + area.y * scale_.y;
    const double x1 = translation_.x + area.right() * scale_.x;
    const double y1 = translation_.y + area.bottom() * scale_.y;
    return rect{std::min(x0, x1), std::min(y0, y1), std::abs(x1 - x0), std::abs(y1 - y0)};
}

recti skia_render_context::device_rect(rect area) const {
    const rect transformed = transformed_rect(area);
    return recti{
        static_cast<int>(std::floor(transformed.x)),
        static_cast<int>(std::floor(transformed.y)),
        static_cast<int>(std::ceil(transformed.right())),
        static_cast<int>(std::ceil(transformed.bottom())),
    };
}

void skia_render_context::draw_rect(rect area, fill_mode fill) {
    SkCanvas* canvas = impl_ ? impl_->canvas() : nullptr;
    const rect transformed = transformed_rect(area);
    const fcolor normalized = fill.color;
    if (!canvas || clip_.empty() || transformed.empty() || normalized.a <= 0.0f) {
        return;
    }

    SkPaint paint;
    paint.setAntiAlias(false);
    paint.setColor4f({normalized.r, normalized.g, normalized.b, normalized.a});
    paint.setBlendMode(SkBlendMode::kSrcOver);

    canvas->drawRect(
        SkRect::MakeXYWH(static_cast<float>(transformed.x), static_cast<float>(transformed.y),
                         static_cast<float>(transformed.w), static_cast<float>(transformed.h)),
        paint);
}

void skia_render_context::draw_shaped_text(
    px_font_t* font, vec2 position, color value, fx_layout* layout, bool subpixel_positioning) {
    const fcolor normalized = value;
    if (!valid() || !font || !font->font || !layout || layout->glyphs.empty() ||
        normalized.a <= 0.0f) {
        return;
    }

    // Sublime's software text path obtains writable pixels from the Skia device and blends its own
    // fx_glyph_cache output. Keep that division here: Skia never reshapes or rerasterizes text.
    impl_->pixels_will_change();

    const float raster_scale = std::max(0.01f, static_cast<float>(std::abs(scale_.x)));
    fx_glyph_cache& cache = font->glyph_cache(raster_scale);
    const double device_origin_x = translation_.x + position.x * scale_.x;
    const double device_origin_y = translation_.y + position.y * scale_.y;
    const float lightness =
        (std::max({normalized.r, normalized.g, normalized.b}) +
         std::min({normalized.r, normalized.g, normalized.b})) *
        0.5f;
    const bool alternate = lightness > 0.75f;
    const uint8_t tint[] = {value.blue(), value.green(), value.red(), value.alpha()};

    auto* destination = static_cast<uint8_t*>(buffer_.pixels);
    for (const fx_glyph& glyph : layout->glyphs) {
        const double x = device_origin_x + static_cast<double>(glyph.x_offset) * scale_.x;
        const double y = device_origin_y + static_cast<double>(glyph.y_offset) * scale_.y;

        const int device_x = static_cast<int>(std::floor(x));
        const double fraction = x - std::floor(x);
        const int phase =
            subpixel_positioning ? std::clamp(static_cast<int>(fraction * 6.0), 0, 5) : 0;
        const fx_glyph_bitmap& bitmap =
            cache.lookup_glyph_data(glyph.id, static_cast<unsigned>(phase), alternate);
        if (bitmap.empty() ||
            bitmap.width > static_cast<size_t>(std::numeric_limits<int>::max()) ||
            bitmap.height > static_cast<size_t>(std::numeric_limits<int>::max())) {
            continue;
        }

        const int glyph_left = device_x + bitmap.bearing_x;
        const int glyph_top = static_cast<int>(std::ceil(y - 0.5)) + bitmap.bearing_y;
        const int bitmap_width = static_cast<int>(bitmap.width);
        const int bitmap_height = static_cast<int>(bitmap.height);
        const int left = std::max(clip_.left, glyph_left);
        const int top = std::max(clip_.top, glyph_top);
        const int right = std::min(clip_.right, glyph_left + bitmap_width);
        const int bottom = std::min(clip_.bottom, glyph_top + bitmap_height);
        if (left >= right || top >= bottom) {
            continue;
        }

        for (int destination_y = top; destination_y < bottom; ++destination_y) {
            const int source_y = destination_y - glyph_top;
            uint8_t* dst = destination + static_cast<size_t>(destination_y) * buffer_.row_bytes +
                           static_cast<size_t>(left) * 4u;
            const uint8_t* src =
                bitmap.pixels.data() + (static_cast<size_t>(source_y) * bitmap.width +
                                        static_cast<size_t>(left - glyph_left)) *
                                           4u;
            composite_glyph_scanline(dst, src, right - left, tint, bitmap.colored, alternate);
        }
    }
}

void skia_render_context::translate(double x, double y) {
    translation_.x += x * scale_.x;
    translation_.y += y * scale_.y;
}

void skia_render_context::scale(double x, double y) {
    scale_.x *= x;
    scale_.y *= y;
}

void skia_render_context::restrict_clip_rect(rect area) {
    clip_ = intersect_recti(clip_, device_rect(area));
    if (SkCanvas* canvas = impl_ ? impl_->canvas() : nullptr; canvas && !clip_.empty()) {
        canvas->clipRect(
            SkRect::MakeLTRB(static_cast<float>(clip_.left), static_cast<float>(clip_.top),
                             static_cast<float>(clip_.right), static_cast<float>(clip_.bottom)));
    }
}

void skia_render_context::push_state(bool preserve_batch) {
    state_stack_.push_back(saved_state{translation_, scale_, clip_});
    if (SkCanvas* canvas = impl_ ? impl_->canvas() : nullptr) {
        canvas->save();
    }
}

void skia_render_context::pop_state() {
    if (state_stack_.empty()) {
        return;
    }
    const saved_state state = state_stack_.back();
    state_stack_.pop_back();
    translation_ = state.translation;
    scale_ = state.scale;
    clip_ = state.clip;
    if (SkCanvas* canvas = impl_ ? impl_->canvas() : nullptr) {
        canvas->restore();
    }
}
