#include "base/apple/scoped_cftyperef.h"
#include "base/apple/scoped_cgtyperef.h"
#include "base/numeric/safe_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "base/unicode/utf16_to_utf8_indices_map.h"
#include "fx/fx.h"

#include <CoreText/CoreText.h>
#include <cmath>
#include <cstring>
#include <spdlog/spdlog.h>
#include <string>
#include <utility>

using base::apple::OwnershipPolicy;
using base::apple::ScopedCFTypeRef;
using base::apple::ScopedCGColorSpace;
using base::apple::ScopedCGContext;

namespace {

const fx_gamma_ramp* identity_gamma_ramp();

class core_text_font final : public fx_font {
public:
    static std::unique_ptr<core_text_font> create(std::string family, float size, uint32_t attrs);

    uint32_t attrs() const override { return attrs_; }
    fx_font_metrics metrics() const override;
    float raster_ascent() const override;
    std::unique_ptr<fx_layout> shape(std::string_view utf8) override;
    std::unique_ptr<fx_layout> shape(std::u32string_view utf32) override;
    void extents(uint32_t glyph, float scale, vec2& origin, vec2& size) override;
    fx_glyph_bitmap rasterise(uint32_t glyph, vec2 subpixel_offset, float scale) override;
    bool is_color_glyph(uint32_t glyph) override;
    bool bg_affects_rasterise() const override { return true; }
    const fx_gamma_ramp* gamma_ramp() const override { return identity_gamma_ramp(); }

private:
    core_text_font(ScopedCFTypeRef<CTFontRef> primary, float requested_size, uint32_t attrs)
        : requested_size_(requested_size), attrs_(attrs) {
        faces_.push_back(std::move(primary));
    }

    uint32_t register_face(CTFontRef face);
    CTFontRef primary() const { return faces_.front().get(); }

    float requested_size_ = 0.0f;
    uint32_t attrs_ = 0;
    // faces_[0] is the requested font and the remaining entries are shaping fallbacks. The binary
    // keeps this append-only vector directly in core_text_font at +0x38.
    std::vector<ScopedCFTypeRef<CTFontRef>> faces_;
};

namespace {

void append_font_feature(CFMutableArrayRef features, int type, int selector) {
    auto type_number =
        ScopedCFTypeRef<CFNumberRef>(CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &type));
    auto selector_number = ScopedCFTypeRef<CFNumberRef>(
        CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &selector));
    const void* keys[] = {kCTFontFeatureTypeIdentifierKey, kCTFontFeatureSelectorIdentifierKey};
    const void* values[] = {type_number.get(), selector_number.get()};
    auto feature = ScopedCFTypeRef<CFDictionaryRef>(
        CFDictionaryCreate(kCFAllocatorDefault, keys, values, 2, &kCFTypeDictionaryKeyCallBacks,
                           &kCFTypeDictionaryValueCallBacks));
    CFArrayAppendValue(features, feature.get());
}

ScopedCFTypeRef<CFArrayRef> make_font_features(uint32_t flags) {
    auto features = ScopedCFTypeRef<CFMutableArrayRef>(
        CFArrayCreateMutable(kCFAllocatorDefault, 4, &kCFTypeArrayCallBacks));
    append_font_feature(features.get(), kLigaturesType, kRequiredLigaturesOnSelector);
    append_font_feature(features.get(), kLigaturesType,
                        flags & FX_FONT_NO_LIGA ? kCommonLigaturesOffSelector
                                                : kCommonLigaturesOnSelector);
    append_font_feature(features.get(), kLigaturesType,
                        flags & FX_FONT_NO_CLIG ? kContextualLigaturesOffSelector
                                                : kContextualLigaturesOnSelector);
    append_font_feature(features.get(), kContextualAlternatesType,
                        flags & FX_FONT_NO_CALT ? kContextualAlternatesOffSelector
                                                : kContextualAlternatesOnSelector);
    return ScopedCFTypeRef<CFArrayRef>(features.release());
}

}  // namespace

// Appends `face` to the font's registry if it isn't already there and returns its index. Sublime
// compares with CFEqual rather than by pointer (0x1002b4544): Core Text can hand back distinct
// CTFontRef instances for the same font, which a pointer compare would register twice.
uint32_t core_text_font::register_face(CTFontRef face) {
    for (size_t i = 0; i < faces_.size(); i++) {
        if (CFEqual(faces_[i].get(), face)) return base::checked_cast<uint32_t>(i);
    }
    faces_.emplace_back(face, OwnershipPolicy::kRetain);
    return base::checked_cast<uint32_t>(faces_.size() - 1);
}

std::unique_ptr<core_text_font> core_text_font::create(std::string family,
                                                       float size_px,
                                                       uint32_t attrs) {
    ScopedCFTypeRef<CTFontRef> ct;
    if (family == "system") {
        ct.reset(CTFontCreateUIFontForLanguage(kCTFontUIFontLabel, size_px, CFSTR("en-US")));
    } else {
        auto ct_family = base::sys_utf8_to_cfstring_ref(family);
        ct.reset(CTFontCreateWithName(ct_family.get(), size_px, nullptr));
    }
    if (!ct) return nullptr;

    auto features = make_font_features(attrs);
    const void* descriptor_keys[] = {kCTFontFeatureSettingsAttribute};
    const void* descriptor_values[] = {features.get()};
    auto descriptor_attributes = ScopedCFTypeRef<CFDictionaryRef>(
        CFDictionaryCreate(kCFAllocatorDefault, descriptor_keys, descriptor_values, 1,
                           &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));
    auto descriptor = ScopedCFTypeRef<CTFontDescriptorRef>(
        CTFontDescriptorCreateWithAttributes(descriptor_attributes.get()));
    if (CTFontRef featured =
            CTFontCreateCopyWithAttributes(ct.get(), 0.0, nullptr, descriptor.get())) {
        ct.reset(featured);
    }

    // Try setting bold/italic. Fall back to default font otherwise.
    CTFontSymbolicTraits traits = 0;
    if (attrs & FX_FONT_BOLD) traits |= kCTFontTraitBold;
    if (attrs & FX_FONT_ITALIC) traits |= kCTFontTraitItalic;
    if (traits) {
        if (CTFontRef styled =
                CTFontCreateCopyWithSymbolicTraits(ct.get(), size_px, nullptr, traits, traits)) {
            ct.reset(styled);
        }
    }
    return std::unique_ptr<core_text_font>(new core_text_font(std::move(ct), size_px, attrs));
}

namespace {

ScopedCFTypeRef<CTLineRef> make_ctline(CTFontRef ctfont,
                                       uint32_t feature_flags,
                                       std::string_view utf8) {
    auto features = make_font_features(feature_flags);
    const void* keys[] = {kCTFontAttributeName, kCTFontFeatureSettingsAttribute};
    const void* vals[] = {ctfont, features.get()};
    auto attrs = ScopedCFTypeRef<CFDictionaryRef>(
        CFDictionaryCreate(kCFAllocatorDefault, keys, vals, 2, &kCFTypeDictionaryKeyCallBacks,
                           &kCFTypeDictionaryValueCallBacks));
    auto text = base::sys_utf8_to_cfstring_ref(utf8);
    auto as = ScopedCFTypeRef<CFAttributedStringRef>(
        CFAttributedStringCreate(kCFAllocatorDefault, text.get(), attrs.get()));
    return ScopedCFTypeRef<CTLineRef>(CTLineCreateWithAttributedString(as.get()));
}

}  // namespace

std::unique_ptr<fx_layout> core_text_font::shape(std::string_view utf8) {
    CTFontRef ctfont = primary();
    auto line = make_ctline(ctfont, attrs_, utf8);

    CFArrayRef runs = CTLineGetGlyphRuns(line.get());
    CFIndex run_count = CFArrayGetCount(runs);

    auto shaped = std::make_unique<fx_layout>();
    shaped->line_height = static_cast<float>(std::ceil(CTFontGetAscent(ctfont)) +
                                             std::ceil(CTFontGetDescent(ctfont)) +
                                             std::ceil(CTFontGetLeading(ctfont)));
    const bool snap_advances = (CTFontGetSymbolicTraits(ctfont) & kCTFontTraitMonoSpace) &&
                               requested_size_ <= 16.0f && !(attrs_ & FX_FONT_NO_ROUND);
    base::UTF16ToUTF8IndicesMap indices_map;
    if (!indices_map.set_utf8(utf8)) {
        // Malformed UTF-8. Shaping still works, but the map is empty and operator[] is unchecked,
        // so the cluster lookup below has to be skipped rather than read out of bounds.
        spdlog::warn("could not map UTF-16 to UTF-8 indices; cluster offsets will be 0");
    }

    for (CFIndex r = 0; r < run_count; r++) {
        CTRunRef run = (CTRunRef)CFArrayGetValueAtIndex(runs, r);
        const size_t n = base::checked_cast<size_t>(CTRunGetGlyphCount(run));
        if (n == 0) continue;

        std::vector<CGGlyph> glyphs(n);
        std::vector<CGPoint> positions(n);
        std::vector<CGSize> advances(n);
        std::vector<CFIndex> indices(n);

        CTRunGetGlyphs(run, CFRangeMake(0, 0), glyphs.data());
        CTRunGetPositions(run, CFRangeMake(0, 0), positions.data());
        CTRunGetAdvances(run, CFRangeMake(0, 0), advances.data());
        CTRunGetStringIndices(run, CFRangeMake(0, 0), indices.data());

        // Run attributes can override the font. Use the run font if present.
        CTFontRef run_font = ctfont;
        CFDictionaryRef run_attrs = CTRunGetAttributes(run);
        if (run_attrs) {
            auto value = CFDictionaryGetValue(run_attrs, kCTFontAttributeName);
            if (value) run_font = static_cast<CTFontRef>(value);
        }

        const uint32_t face = register_face(run_font);
        shaped->glyphs.reserve(shaped->glyphs.size() + n);
        float advance_delta = 0.0f;

        for (size_t i = 0; i < n; i++) {
            // TODO: Handle kCFNotFound case in TextShaper::shape().
            if (indices[i] == kCFNotFound) {
                spdlog::error("TODO: Handle kCFNotFound case in TextShaper::shape()");
                NOTREACHED();
            }

            const size_t utf16_index = base::checked_cast<size_t>(indices[i]);
            const size_t utf8_index =
                utf16_index < indices_map.size() ? indices_map[utf16_index] : 0;
            const float original_advance = static_cast<float>(advances[i].width);
            float x_advance = original_advance;
            if (snap_advances) {
                if (attrs_ & FX_FONT_NO_ANTIALIAS) {
                    const float lower = std::floor(x_advance);
                    x_advance = x_advance - lower < 0.25f ? lower : std::ceil(x_advance);
                } else {
                    x_advance = std::round(x_advance);
                }
                advance_delta += x_advance - original_advance;
            }
            shaped->glyphs.push_back({
                .id = (face << 16) | static_cast<uint32_t>(glyphs[i]),
                .x_offset = static_cast<float>(positions[i].x) + advance_delta,
                // Core Text positions are y-up; negate to the library's y-down convention.
                .y_offset = static_cast<float>(-positions[i].y),
                .cluster = base::checked_cast<uint32_t>(utf8_index),
            });
            shaped->advance += x_advance;
        }
    }
    return shaped;
}

fx_glyph_bitmap core_text_font::rasterise(uint32_t glyph, vec2 subpixel_offset, float scale) {
    const uint32_t face = glyph >> 16;
    if (face >= faces_.size()) return {};

    CTFontRef ctfont = faces_[face].get();
    CGGlyph core_text_glyph = static_cast<uint16_t>(glyph);

    CGRect bbox = CTFontGetBoundingRectsForGlyphs(ctfont, kCTFontOrientationHorizontal,
                                                  &core_text_glyph, nullptr, 1);
    if (CGRectIsEmpty(bbox)) return {};

    // Ink box in device pixels, baseline-relative. ST ceils the extent and rounds the top/left
    // origin independently, then pads a border on every side.
    constexpr int kBorder = 2;
    constexpr int kBytesPerPixel = 4;
    const int ceil_w = base::clamp_ceil<int>(bbox.size.width * scale);
    const int ceil_h = base::clamp_ceil<int>(bbox.size.height * scale);
    const int width = ceil_w + 2 * kBorder + 1;
    const int height = ceil_h + 2 * kBorder;
    const int x0 = base::clamp_round<int>(bbox.origin.x * scale) - kBorder;
    const int y0 =
        base::clamp_round<int>((bbox.origin.y + bbox.size.height) * scale) - ceil_h - kBorder;
    const int bytes_per_row = width * kBytesPerPixel;

    const bool colored = is_color_glyph(glyph);

    std::vector<uint8_t> pixels(base::checked_cast<size_t>(height * bytes_per_row));

    auto color_space = ScopedCGColorSpace(CGColorSpaceCreateDeviceRGB());
    auto context = ScopedCGContext(CGBitmapContextCreate(
        pixels.data(), base::checked_cast<size_t>(width), base::checked_cast<size_t>(height), 8,
        base::checked_cast<size_t>(bytes_per_row), color_space.get(),
        kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host));

    const CGFloat component = colored ? 1.0 : 0.0;
    const CGFloat fill[] = {component, component, component, 1.0};
    CGContextSetFillColorSpace(context.get(), color_space.get());
    CGContextSetFillColor(context.get(), fill);
    CGContextSetShouldAntialias(context.get(), true);
    CGContextSetShouldSmoothFonts(context.get(), true);
    CGContextScaleCTM(context.get(), scale, scale);

    CGPoint position = {(-x0 + subpixel_offset.x) / scale, (-y0 - subpixel_offset.y) / scale};
    CTFontDrawGlyphs(ctfont, &core_text_glyph, &position, 1, context.get());

    if (!colored) {
        // The normal glyph was drawn in black, so coverage sits in alpha and rgb is 0. Spread
        // alpha across rgb; Core Graphics antialiasing is grayscale.
        for (size_t i = 0; i < pixels.size(); i += kBytesPerPixel) {
            const uint8_t coverage = pixels[i + 3];  // host-order BGRA
            pixels[i] = pixels[i + 1] = pixels[i + 2] = coverage;
        }
    }

    int ink_left = width;
    int ink_top = height;
    int ink_right = -1;
    int ink_bottom = -1;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const size_t offset = static_cast<size_t>((y * width + x) * kBytesPerPixel);
            uint32_t pixel = 0;
            std::memcpy(&pixel, pixels.data() + offset, sizeof(pixel));
            if (pixel != 0) {
                ink_left = std::min(ink_left, x);
                ink_top = std::min(ink_top, y);
                ink_right = std::max(ink_right, x);
                ink_bottom = std::max(ink_bottom, y);
            }
        }
    }
    if (ink_right < ink_left || ink_bottom < ink_top) return {};

    const int ink_width = ink_right - ink_left + 1;
    const int ink_height = ink_bottom - ink_top + 1;
    std::vector<uint8_t> cropped(
        base::checked_cast<size_t>(ink_width * ink_height * kBytesPerPixel));
    for (int y = 0; y < ink_height; ++y) {
        const uint8_t* source =
            pixels.data() + ((ink_top + y) * width + ink_left) * kBytesPerPixel;
        uint8_t* destination = cropped.data() + y * ink_width * kBytesPerPixel;
        std::memcpy(destination, source, base::checked_cast<size_t>(ink_width * kBytesPerPixel));
    }

    return {
        .width = base::checked_cast<size_t>(ink_width),
        .height = base::checked_cast<size_t>(ink_height),
        .bearing_x = x0 + ink_left,
        // Core Text positions are y-up; negate to the library's y-down convention.
        .bearing_y = -(y0 + height) + ink_top,
        .colored = colored,
        .pixels = std::move(cropped),
    };
}

std::string utf32_to_utf8(std::u32string_view input) {
    std::string output;
    output.reserve(input.size());
    for (uint32_t cp : input) {
        if (cp <= 0x7f) {
            output.push_back(static_cast<char>(cp));
        } else if (cp <= 0x7ff) {
            output.push_back(static_cast<char>(0xc0 | (cp >> 6)));
            output.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
        } else if (cp <= 0xffff) {
            output.push_back(static_cast<char>(0xe0 | (cp >> 12)));
            output.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3f)));
            output.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
        } else if (cp <= 0x10ffff) {
            output.push_back(static_cast<char>(0xf0 | (cp >> 18)));
            output.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3f)));
            output.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3f)));
            output.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
        }
    }
    return output;
}

const fx_gamma_ramp* identity_gamma_ramp() {
    static const fx_gamma_ramp ramp = [] {
        fx_gamma_ramp result;
        for (size_t i = 0; i < result.values.size(); ++i) {
            result.values[i] = static_cast<uint8_t>(i);
            result.inverse_values[i] = static_cast<uint8_t>(i);
        }
        return result;
    }();
    return &ramp;
}

fx_font_metrics core_text_font::metrics() const {
    const float ascent = static_cast<float>(std::ceil(CTFontGetAscent(primary())));
    const float descent = static_cast<float>(std::ceil(CTFontGetDescent(primary())));
    const float leading = static_cast<float>(std::ceil(CTFontGetLeading(primary())));
    return {
        .ascent = ascent,
        .descent = descent,
        .leading = leading,
        .line_height = ascent + descent + leading,
    };
}

float core_text_font::raster_ascent() const {
    return static_cast<float>(std::ceil(CTFontGetAscent(primary())));
}

std::unique_ptr<fx_layout> core_text_font::shape(std::u32string_view utf32) {
    return shape(utf32_to_utf8(utf32));
}

void core_text_font::extents(uint32_t glyph, float scale, vec2& origin, vec2& size) {
    const uint32_t face = glyph >> 16;
    if (face >= faces_.size()) {
        origin = {};
        size = {};
        return;
    }

    CTFontRef ctfont = faces_[face].get();
    CGGlyph core_text_glyph = static_cast<uint16_t>(glyph);
    const CGRect bounds = CTFontGetBoundingRectsForGlyphs(ctfont, kCTFontOrientationHorizontal,
                                                          &core_text_glyph, nullptr, 1);
    if (CGRectIsEmpty(bounds)) {
        origin = {};
        size = {};
        return;
    }

    constexpr double kBorder = 2.0;
    origin = {
        std::round(-bounds.origin.x * scale) + kBorder,
        std::round((bounds.origin.y + bounds.size.height - CTFontGetAscent(primary())) * scale) +
            kBorder,
    };
    size = {
        std::ceil(bounds.size.width * scale) + 2.0 * kBorder,
        std::ceil(bounds.size.height * scale) + 2.0 * kBorder,
    };
}

bool core_text_font::is_color_glyph(uint32_t glyph) {
    const uint32_t face = glyph >> 16;
    if (face >= faces_.size()) {
        return false;
    }

    CTFontRef ctfont = faces_[face].get();
    if (!(CTFontGetSymbolicTraits(ctfont) & kCTFontTraitColorGlyphs)) {
        return false;
    }

    const CGGlyph core_text_glyph = static_cast<uint16_t>(glyph);
    auto outline_path =
        ScopedCFTypeRef<CGPathRef>(CTFontCreatePathForGlyph(ctfont, core_text_glyph, nullptr));
    return !outline_path;
}

}  // namespace

std::unique_ptr<fx_font> fx_create_font(std::string_view family, float size, uint32_t attrs) {
    return core_text_font::create(std::string(family), size, attrs);
}
