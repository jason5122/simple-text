#include "base/apple/scoped_cftyperef.h"
#include "base/apple/scoped_cgtyperef.h"
#include "base/numeric/safe_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "base/unicode/utf16_to_utf8_indices_map.h"
#include "experiments/rasterizer/font.h"
#include <CoreText/CoreText.h>
#include <spdlog/spdlog.h>
#include <utility>

using base::apple::OwnershipPolicy;
using base::apple::ScopedCFTypeRef;
using base::apple::ScopedCGColorSpace;
using base::apple::ScopedCGContext;

namespace font {

struct FontHandle::Impl {
    ScopedCFTypeRef<CTFontRef> ctfont;
};

FontHandle::~FontHandle() = default;
FontHandle::FontHandle(FontHandle&& other) = default;
FontHandle& FontHandle::operator=(FontHandle&& other) = default;

FontHandle::FontHandle(CTFontRef ct_font) : impl_(std::make_unique<Impl>()) {
    impl_->ctfont = ScopedCFTypeRef<CTFontRef>(ct_font, OwnershipPolicy::kRetain);
}

bool FontHandle::valid() const { return impl_ && impl_->ctfont.get() != nullptr; }
double FontHandle::ascent() const { return CTFontGetAscent(impl_->ctfont.get()); }
double FontHandle::descent() const { return CTFontGetDescent(impl_->ctfont.get()); }
double FontHandle::leading() const { return CTFontGetLeading(impl_->ctfont.get()); }
const void* FontHandle::native_handle() const { return impl_->ctfont.get(); }
CTFontRef FontHandle::ct_font() const { return impl_->ctfont.get(); }

std::optional<font::FontHandle> create_font(std::string family,
                                            double size_px,
                                            Weight weight,
                                            Slant slant) {
    ScopedCFTypeRef<CTFontRef> ct;
    if (family == "system") {
        ct.reset(CTFontCreateUIFontForLanguage(kCTFontUIFontLabel, size_px, CFSTR("en-US")));
    } else {
        auto ct_family = base::sys_utf8_to_cfstring_ref(family);
        ct.reset(CTFontCreateWithName(ct_family.get(), size_px, nullptr));
    }
    if (!ct) return std::nullopt;

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
    return FontHandle(ct.get());
}

ShapedLine shape(const FontHandle& font, std::string_view utf8) {
    auto ctfont = static_cast<CTFontRef>(font.native_handle());

    // TODO: Is disabling kerning correct? Sublime Text seems to lay out as if kerning is disabled,
    // but I'm not sure if they literally disable kerning or if they just make glyphs context-free.
    double kern_zero = 0.0;
    auto kern = ScopedCFTypeRef<CFNumberRef>(
        CFNumberCreate(kCFAllocatorDefault, kCFNumberDoubleType, &kern_zero));
    const void* keys[] = {kCTFontAttributeName, kCTKernAttributeName};
    const void* vals[] = {ctfont, kern.get()};
    auto attrs = ScopedCFTypeRef<CFDictionaryRef>(
        CFDictionaryCreate(kCFAllocatorDefault, keys, vals, 2, &kCFTypeDictionaryKeyCallBacks,
                           &kCFTypeDictionaryValueCallBacks));
    auto text = base::sys_utf8_to_cfstring_ref(utf8);
    auto as = ScopedCFTypeRef<CFAttributedStringRef>(
        CFAttributedStringCreate(kCFAllocatorDefault, text.get(), attrs.get()));
    auto line = ScopedCFTypeRef<CTLineRef>(CTLineCreateWithAttributedString(as.get()));

    CFArrayRef runs = CTLineGetGlyphRuns(line.get());
    CFIndex run_count = CFArrayGetCount(runs);

    std::vector<ShapedRun> shaped_runs;
    shaped_runs.reserve(base::checked_cast<size_t>(run_count));
    base::UTF16ToUTF8IndicesMap indices_map;
    indices_map.set_utf8(utf8);

    // Line-level accumulators (LTR-only). pen_x is the total advance, for the returned width.
    // mono_delta accumulates the monospace advance-rounding nudge (round(adv)-adv), added on top
    // of each glyph's shaped position below.
    double pen_x = 0;
    double mono_delta = 0;

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

        // Small-size layout half of Sublime's behavior (the rendering half is 6-phase sub-pixel
        // positioning in draw_text): snap a monospace font's advance to a whole point so columns
        // land on a pixel grid (e.g. Source Code Pro 9.6pt -> 10pt). Monospace only --
        // proportional faces keep their fractional advances. Rounding is in points, before the
        // device scale.
        bool snap_advance = (CTFontGetSymbolicTraits(run_font) & kCTFontTraitMonoSpace) &&
                            CTFontGetSize(run_font) <= kMonospaceSnapMaxPt;

        std::vector<GlyphPlacement> glyph_placements;
        glyph_placements.reserve(n);

        for (size_t i = 0; i < n; i++) {
            // TODO: Handle kCFNotFound case in TextShaper::shape().
            if (indices[i] == kCFNotFound) {
                spdlog::error("TODO: Handle kCFNotFound case in TextShaper::shape()");
                NOTREACHED();
            }

            double shaped = advances[i].width;
            double x_advance = snap_advance ? std::round(shaped) : shaped;
            mono_delta += x_advance - shaped;

            size_t utf8_index = indices_map[base::checked_cast<size_t>(indices[i])];
            glyph_placements.push_back({
                .glyph_id = glyphs[i],
                .x_advance = x_advance,
                // TODO: `x_offset` is a misleading name because it carries absolute x positions.
                .x_offset = positions[i].x + mono_delta,
                // Core Text positions are y-up; negate to the library's y-down convention.
                .y_offset = -positions[i].y,
                .cluster = utf8_index,
            });
            pen_x += x_advance;
        }

        shaped_runs.emplace_back(FontHandle(run_font), glyph_placements);
    }

    return {.runs = std::move(shaped_runs), .width = pen_x};
}

GlyphBitmap rasterize(const FontHandle& font, GlyphId glyph, double s, double subpixel_x) {
    auto ctfont = static_cast<CTFontRef>(font.native_handle());
    CGGlyph g = base::checked_cast<CGGlyph>(glyph);  // Core Text glyph ids are 16-bit

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
    const int w = ceil_w + 2 * kBorder;
    const int h = ceil_h + 2 * kBorder;
    const int x0 = base::clamp_round<int>(bbox.origin.x * s) - kBorder;
    const int y0 =
        base::clamp_round<int>((bbox.origin.y + bbox.size.height) * s) - ceil_h - kBorder;
    const int bytes_per_row = w * kBytesPerPixel;

    std::vector<uint8_t> pixels(base::checked_cast<size_t>(h * bytes_per_row));

    auto cs = ScopedCGColorSpace(CGColorSpaceCreateDeviceRGB());
    auto ctx = ScopedCGContext(CGBitmapContextCreate(
        pixels.data(), base::checked_cast<size_t>(w), base::checked_cast<size_t>(h), 8,
        base::checked_cast<size_t>(bytes_per_row), cs.get(),
        kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host));

    CGContextSetShouldAntialias(ctx.get(), true);
    CGContextSetShouldSmoothFonts(ctx.get(), true);
    CGContextScaleCTM(ctx.get(), s, s);
    CGContextSetGrayFillColor(ctx.get(), 0.0, 1.0);

    // subpixel_x nudges the draw position by a fraction of a device pixel (converted to points),
    // baking the glyph's horizontal phase into the antialiasing. The 2px border absorbs the shift,
    // so bearing_x/bearing_y stay unchanged.
    CGPoint pos = {(-x0 + subpixel_x) / s, -y0 / s};
    CTFontDrawGlyphs(ctfont, &g, &pos, 1, ctx.get());

    // Color fonts (emoji) draw real RGBA that must not be tinted; monochrome fonts draw a coverage
    // mask. This is a font-level trait -- fine for emoji, where every glyph is colored.
    const bool is_color = (CTFontGetSymbolicTraits(ctfont) & kCTFontTraitColorGlyphs) != 0;

    return {
        .width = base::checked_cast<size_t>(w),
        .height = base::checked_cast<size_t>(h),
        // Convert the y-up raster origin to the library's top-left convention: x is unchanged (no
        // flip); bearing_y flips the bottom edge (y0, y-up) to the top edge (y-down), so it is
        // negative when the glyph rises above the baseline.
        .bearing_x = x0,
        .bearing_y = -(y0 + h),
        .is_color = is_color,
        .pixels = std::move(pixels),
    };
}

}  // namespace font
