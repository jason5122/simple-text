#include "base/apple/scoped_cftyperef.h"
#include "base/apple/scoped_cgtyperef.h"
#include "base/numeric/safe_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "base/unicode/utf16_to_utf8_indices_map.h"
#include "experiments/rasterizer/font.h"
#include <CoreText/CoreText.h>
#include <map>
#include <spdlog/spdlog.h>
#include <utility>

using base::apple::OwnershipPolicy;
using base::apple::ScopedCFTypeRef;
using base::apple::ScopedCGColorSpace;
using base::apple::ScopedCGContext;

namespace font {

class FontDatabase::Impl {
public:
    struct Face {
        bool is_system = false;
        ScopedCFTypeRef<CFStringRef> name;  // family/PostScript name; unset when is_system
        CTFontSymbolicTraits traits = 0;    // bold/italic, applied in create_font()
    };

    std::vector<Face> faces;  // FontFaceId indexes this
    std::map<std::pair<std::string, CTFontSymbolicTraits>, FontFaceId> key_to_id;
};

FontDatabase::FontDatabase() : impl_(std::make_unique<Impl>()) {}

FontDatabase::~FontDatabase() = default;

std::optional<FontFaceId> FontDatabase::match(const FontRequest& request) {
    CTFontSymbolicTraits traits = 0;
    if (request.weight == Weight::Bold) traits |= kCTFontTraitBold;
    if (request.slant == Slant::Italic) traits |= kCTFontTraitItalic;

    auto key = std::make_pair(request.family, traits);
    if (auto it = impl_->key_to_id.find(key); it != impl_->key_to_id.end()) return it->second;

    Impl::Face face;
    face.is_system = request.family.empty() || request.family == "system";
    face.traits = traits;
    if (!face.is_system) face.name = base::sys_utf8_to_cfstring_ref(request.family);

    const FontFaceId id = base::checked_cast<FontFaceId>(impl_->faces.size());
    impl_->faces.push_back(std::move(face));
    impl_->key_to_id.emplace(std::move(key), id);
    return id;
}

struct FontHandle::Impl {
    ScopedCFTypeRef<CTFontRef> ctfont;
};

FontHandle::~FontHandle() = default;
FontHandle::FontHandle(FontHandle&& other) = default;
FontHandle& FontHandle::operator=(FontHandle&& other) = default;

bool FontHandle::valid() const { return impl_ && impl_->ctfont.get() != nullptr; }
double FontHandle::ascent() const { return CTFontGetAscent(impl_->ctfont.get()); }
double FontHandle::descent() const { return CTFontGetDescent(impl_->ctfont.get()); }
double FontHandle::leading() const { return CTFontGetLeading(impl_->ctfont.get()); }

std::optional<font::FontHandle> FontDatabase::create_font(FontFaceId face, double size_px) {
    if (face >= impl_->faces.size()) return std::nullopt;
    const Impl::Face& f = impl_->faces[face];

    ScopedCFTypeRef<CTFontRef> ct;
    if (f.is_system) {
        ct = ScopedCFTypeRef<CTFontRef>(
            CTFontCreateUIFontForLanguage(kCTFontUIFontLabel, size_px, CFSTR("en-US")));
    } else {
        ct = ScopedCFTypeRef<CTFontRef>(CTFontCreateWithName(f.name.get(), size_px, nullptr));
    }
    if (!ct) return std::nullopt;

    // Try setting bold/italic. Fall back to default font otherwise.
    if (f.traits) {
        auto styled = ScopedCFTypeRef<CTFontRef>(
            CTFontCreateCopyWithSymbolicTraits(ct.get(), size_px, nullptr, f.traits, f.traits));
        if (styled) ct = std::move(styled);
    }

    FontHandle out;
    out.impl_ = std::make_unique<FontHandle::Impl>();
    out.impl_->ctfont = std::move(ct);
    return out;
}

ShapedLine TextShaper::shape(const FontHandle& font, std::string_view utf8) const {
    CTFontRef ctfont = font.impl_->ctfont.get();

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
            if (v) run_font = (CTFontRef)v;
        }

        // Small-size layout half of Sublime's behavior (the rendering half is 6-phase sub-pixel
        // positioning in draw_text): snap a monospace font's advance to a whole point so columns
        // land on a pixel grid (e.g. Source Code Pro 9.6pt -> 10pt). Monospace only --
        // proportional faces keep their fractional advances. Rounding is in points, before the
        // device scale.
        bool snap_advance = (CTFontGetSymbolicTraits(run_font) & kCTFontTraitMonoSpace) &&
                            CTFontGetSize(run_font) <= kSmallSizeThresholdPt;

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
                .y_advance = advances[i].height,
                .x_offset = positions[i].x + mono_delta,
                // Core Text positions are y-up; negate to the library's y-down convention.
                .y_offset = -positions[i].y,
                .cluster = utf8_index,
            });
            pen_x += x_advance;
        }

        FontHandle handle;
        handle.impl_ = std::make_unique<FontHandle::Impl>();
        handle.impl_->ctfont = ScopedCFTypeRef<CTFontRef>(run_font, OwnershipPolicy::kRetain);
        shaped_runs.emplace_back(std::move(handle), glyph_placements);
    }

    return {
        .runs = std::move(shaped_runs),
        .width = pen_x,
    };
}

GlyphBitmap GlyphRasterizer::rasterize(const FontHandle& font,
                                       GlyphId glyph,
                                       double s,
                                       double subpixel_x) const {
    CTFontRef ctfont = font.impl_->ctfont.get();
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

    return {
        .width = base::checked_cast<size_t>(w),
        .height = base::checked_cast<size_t>(h),
        // Convert the y-up raster origin to the library's top-left convention: x is unchanged (no
        // flip); bearing_y flips the bottom edge (y0, y-up) to the top edge (y-down), so it is
        // negative when the glyph rises above the baseline.
        .bearing_x = x0,
        .bearing_y = -(y0 + h),
        .pixels = std::move(pixels),
    };
}

}  // namespace font
