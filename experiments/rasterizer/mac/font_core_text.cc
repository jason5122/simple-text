#include "base/apple/scoped_cftyperef.h"
#include "base/apple/scoped_cgtyperef.h"
#include "base/numeric/safe_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "base/unicode/utf16_to_utf8_indices_map.h"
#include "experiments/rasterizer/font.h"
#include <CoreText/CoreText.h>
#include <cmath>
#include <cstring>
#include <spdlog/spdlog.h>
#include <utility>

using base::apple::OwnershipPolicy;
using base::apple::ScopedCFTypeRef;
using base::apple::ScopedCGColorSpace;
using base::apple::ScopedCGContext;

namespace font {

struct FontHandle::FontData {
    // faces[0] is what create_font resolved to; 1.. are the fallbacks shaping discovered. Sublime
    // keeps the same append-only vector (core_text_font+0x38) and never reorders or evicts it,
    // which is what makes a bare index safe as a cache key for the font's lifetime.
    std::vector<ScopedCFTypeRef<CTFontRef>> faces;
    FontId id = 0;
    uint32_t feature_flags = 0;
    float requested_size = 0.0f;
    std::optional<bool> monospace;

    CTFontRef primary() const { return faces.front().get(); }
};

struct FontHandle::Impl {
    std::shared_ptr<FontData> data;
};

FontHandle::FontHandle() = default;
FontHandle::~FontHandle() = default;
FontHandle::FontHandle(FontHandle&& other) = default;
FontHandle& FontHandle::operator=(FontHandle&& other) = default;

FontHandle::FontHandle(std::shared_ptr<FontData> data)
    : impl_(std::make_unique<Impl>(std::move(data))) {}

FontHandle::FontData& FontHandle::data() const { return *impl_->data; }

bool FontHandle::valid() const { return impl_ && impl_->data && !impl_->data->faces.empty(); }
FontId FontHandle::id() const { return impl_->data->id; }
double FontHandle::ascent() const { return CTFontGetAscent(impl_->data->primary()); }
double FontHandle::descent() const { return CTFontGetDescent(impl_->data->primary()); }
double FontHandle::leading() const { return CTFontGetLeading(impl_->data->primary()); }
double FontHandle::size() const { return CTFontGetSize(impl_->data->primary()); }

bool FontHandle::is_monospace() const {
    if (!impl_->data->monospace) {
        impl_->data->monospace =
            std::abs(shape(*this, "i").advance - shape(*this, "M").advance) < 0.001;
    }
    return *impl_->data->monospace;
}

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
    append_font_feature(features.get(), 1, 0);
    append_font_feature(features.get(), 1, flags & 0x800 ? 3 : 2);
    append_font_feature(features.get(), 1, flags & 0x1000 ? 19 : 18);
    append_font_feature(features.get(), 36, static_cast<int>((flags >> 13) & 1));
    return ScopedCFTypeRef<CFArrayRef>(features.release());
}

// Appends `face` to the font's registry if it isn't already there and returns its index. Sublime
// compares with CFEqual rather than by pointer (0x1002b4544): Core Text can hand back distinct
// CTFontRef instances for the same font, which a pointer compare would register twice.
FontFaceId register_face(FontHandle::FontData& data, CTFontRef face) {
    for (size_t i = 0; i < data.faces.size(); i++) {
        if (CFEqual(data.faces[i].get(), face)) return static_cast<FontFaceId>(i);
    }
    data.faces.emplace_back(face, OwnershipPolicy::kRetain);
    return static_cast<FontFaceId>(data.faces.size() - 1);
}

}  // namespace

std::optional<font::FontHandle> create_font(std::string family,
                                            double size_px,
                                            Weight weight,
                                            Slant slant) {
    return create_font(std::move(family), size_px, weight, slant, 0);
}

std::optional<font::FontHandle> create_font(
    std::string family, double size_px, Weight weight, Slant slant, uint32_t feature_flags) {
    ScopedCFTypeRef<CTFontRef> ct;
    if (family == "system") {
        ct.reset(CTFontCreateUIFontForLanguage(kCTFontUIFontLabel, size_px, CFSTR("en-US")));
    } else {
        auto ct_family = base::sys_utf8_to_cfstring_ref(family);
        ct.reset(CTFontCreateWithName(ct_family.get(), size_px, nullptr));
    }
    if (!ct) return std::nullopt;

    auto features = make_font_features(feature_flags);
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
    if (weight == Weight::Bold) traits |= kCTFontTraitBold;
    if (slant == Slant::Italic) traits |= kCTFontTraitItalic;
    if (traits) {
        if (CTFontRef styled =
                CTFontCreateCopyWithSymbolicTraits(ct.get(), size_px, nullptr, traits, traits)) {
            ct.reset(styled);
        }
    }
    auto data = std::make_shared<FontHandle::FontData>();
    // Process-unique and never reused: a glyph key outliving its font must fail to match, not
    // collide with whatever font is created next.
    static FontId next_id = 1;
    data->id = next_id++;
    data->feature_flags = feature_flags;
    data->requested_size = static_cast<float>(size_px);
    data->faces.emplace_back(ct.get(), OwnershipPolicy::kRetain);
    return FontHandle(std::move(data));
}

std::optional<font::FontHandle> create_font(const FontSpec& spec) {
    return create_font(spec.family, spec.size, spec.weight, spec.slant);
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

void set_debug_use_analysis_path(bool) {}
void set_debug_rendering_params(float, float) {}

std::string rasterizer_debug_info() { return "Core Graphics, grayscale antialiasing"; }

ShapedText shape(const FontHandle& font, std::string_view utf8) {
    FontHandle::FontData& data = font.data();
    CTFontRef ctfont = data.primary();
    auto line = make_ctline(ctfont, data.feature_flags, utf8);

    CFArrayRef runs = CTLineGetGlyphRuns(line.get());
    CFIndex run_count = CFArrayGetCount(runs);

    ShapedText shaped;
    shaped.line_height = static_cast<float>(std::ceil(CTFontGetAscent(ctfont)) +
                                            std::ceil(CTFontGetDescent(ctfont)) +
                                            std::ceil(CTFontGetLeading(ctfont)));
    const bool snap_advances = (CTFontGetSymbolicTraits(ctfont) & kCTFontTraitMonoSpace) &&
                               data.requested_size <= 16.0f && !(data.feature_flags & 0x80);
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
            auto v = CFDictionaryGetValue(run_attrs, kCTFontAttributeName);
            if (v) run_font = static_cast<CTFontRef>(v);
        }

        const FontFaceId face = register_face(data, run_font);
        shaped.glyphs.reserve(shaped.glyphs.size() + n);
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
                if (data.feature_flags & 0x4) {
                    const float lower = std::floor(x_advance);
                    x_advance = x_advance - lower < 0.25f ? lower : std::ceil(x_advance);
                } else {
                    x_advance = std::round(x_advance);
                }
                advance_delta += x_advance - original_advance;
            }
            shaped.glyphs.push_back({
                .glyph_id = pack_glyph(face, glyphs[i]),
                .x_advance = x_advance,
                .x_offset = static_cast<float>(positions[i].x) + advance_delta,
                // Core Text positions are y-up; negate to the library's y-down convention.
                .y_offset = static_cast<float>(-positions[i].y),
                .cluster = utf8_index,
            });
            shaped.advance += x_advance;
        }
    }
    return shaped;
}

GlyphBitmap rasterize(const FontHandle& font, GlyphId glyph, double s, double subpixel_x) {
    const FontHandle::FontData& data = font.data();
    const FontFaceId face = face_index_of(glyph);
    if (face >= data.faces.size()) return {};

    CTFontRef ctfont = data.faces[face].get();
    CGGlyph g = glyph_index_of(glyph);  // Core Text glyph ids are 16-bit

    CGRect bbox =
        CTFontGetBoundingRectsForGlyphs(ctfont, kCTFontOrientationHorizontal, &g, nullptr, 1);
    if (CGRectIsEmpty(bbox)) return {};

    // Ink box in device pixels, baseline-relative. The tiny raster context below is Core
    // Graphics-native (y-up, origin bottom-left), so x0/y0 are the bitmap's bottom-left corner in
    // that space. ST ceils the extent and rounds the top/left origin independently, then pads a
    // border on every side. The per-line ascent that ST folds into its offset cancels for a
    // single-glyph bitmap. Geometry stays in int (offsets can be negative); we cross to size_t
    // only at the vector/Core Graphics boundary below.
    constexpr int kBorder = 2;
    constexpr int kBytesPerPixel = 4;
    const int ceil_w = base::clamp_ceil<int>(bbox.size.width * s);
    const int ceil_h = base::clamp_ceil<int>(bbox.size.height * s);
    const int w = ceil_w + 2 * kBorder + 1;
    const int h = ceil_h + 2 * kBorder;
    const int x0 = base::clamp_round<int>(bbox.origin.x * s) - kBorder;
    const int y0 =
        base::clamp_round<int>((bbox.origin.y + bbox.size.height) * s) - ceil_h - kBorder;
    const int bytes_per_row = w * kBytesPerPixel;

    // fx_rasterise_glyph asks this before invoking core_text_font::rasterise. Its color-glyph
    // branch clears the destination to transparent and passes opaque white as the draw color;
    // monochrome glyphs use black so their alpha can be recovered as coverage below.
    const bool colored_font = CTFontGetSymbolicTraits(ctfont) & kCTFontTraitColorGlyphs;
    auto outline_path = ScopedCFTypeRef<CGPathRef>(CTFontCreatePathForGlyph(ctfont, g, nullptr));
    const bool colored = colored_font && !outline_path;

    std::vector<uint8_t> pixels(base::checked_cast<size_t>(h * bytes_per_row));

    auto cs = ScopedCGColorSpace(CGColorSpaceCreateDeviceRGB());
    auto ctx = ScopedCGContext(CGBitmapContextCreate(
        pixels.data(), base::checked_cast<size_t>(w), base::checked_cast<size_t>(h), 8,
        base::checked_cast<size_t>(bytes_per_row), cs.get(),
        kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host));

    const CGFloat component = colored ? 1.0 : 0.0;
    const CGFloat fill[] = {component, component, component, 1.0};
    // Sublime makes DeviceRGB the current fill space as well as the bitmap's storage space before
    // passing its four-component color to CGContextSetFillColor.
    CGContextSetFillColorSpace(ctx.get(), cs.get());
    CGContextSetFillColor(ctx.get(), fill);
    CGContextSetShouldAntialias(ctx.get(), true);
    CGContextSetShouldSmoothFonts(ctx.get(), true);
    CGContextScaleCTM(ctx.get(), s, s);

    // subpixel_x nudges the draw position by a fraction of a device pixel (converted to points),
    // baking the glyph's horizontal phase into the antialiasing. The 2px border absorbs the shift,
    // so bearing_x/bearing_y stay unchanged.
    CGPoint pos = {(-x0 + subpixel_x) / s, -y0 / s};
    CTFontDrawGlyphs(ctfont, &g, &pos, 1, ctx.get());

    if (!colored) {
        // The normal glyph was drawn in black, so coverage sits in alpha and rgb is 0.
        // GlyphBitmap carries per-channel coverage (DirectWrite's ClearType puts a different one
        // in each channel), so spread alpha across rgb. Core Graphics antialiasing is grayscale,
        // making all three channels equal. Color glyphs instead retain Core Graphics' native
        // premultiplied BGRA; Sublime's COLORED shader samples that value directly and uses its
        // alpha as coverage.
        for (size_t i = 0; i < pixels.size(); i += kBytesPerPixel) {
            const uint8_t coverage = pixels[i + 3];  // host-order BGRA
            pixels[i] = pixels[i + 1] = pixels[i + 2] = coverage;
        }
    }

    int ink_left = w;
    int ink_top = h;
    int ink_right = -1;
    int ink_bottom = -1;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const size_t offset = static_cast<size_t>((y * w + x) * kBytesPerPixel);
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
    if (ink_right < ink_left || ink_bottom < ink_top) {
        return {};
    }

    const int ink_width = ink_right - ink_left + 1;
    const int ink_height = ink_bottom - ink_top + 1;
    std::vector<uint8_t> cropped(
        base::checked_cast<size_t>(ink_width * ink_height * kBytesPerPixel));
    for (int y = 0; y < ink_height; ++y) {
        const uint8_t* source = pixels.data() + ((ink_top + y) * w + ink_left) * kBytesPerPixel;
        uint8_t* destination = cropped.data() + y * ink_width * kBytesPerPixel;
        std::memcpy(destination, source, base::checked_cast<size_t>(ink_width * kBytesPerPixel));
    }

    return {
        .width = base::checked_cast<size_t>(ink_width),
        .height = base::checked_cast<size_t>(ink_height),
        .bearing_x = x0 + ink_left,
        // Core Text positions are y-up; negate to the library's y-down convention.
        .bearing_y = -(y0 + h) + ink_top,
        .colored = colored,
        .pixels = std::move(cropped),
    };
}

}  // namespace font
