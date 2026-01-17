#include "base/apple/scoped_cftyperef.h"
#include "base/apple/scoped_cgtyperef.h"
#include "base/strings/sys_string_conversions.h"
#include "base/unicode/utf16_to_utf8_indices_map.h"
#include "experiments/rasterizer/font.h"
#include <CoreText/CoreText.h>
#include <spdlog/spdlog.h>
#include <unordered_map>

using base::apple::OwnershipPolicy;
using base::apple::ScopedCFTypeRef;
using base::apple::ScopedCGColorSpace;
using base::apple::ScopedCGContext;

namespace font {

class FontDatabase::Impl {
public:
    FontFaceId next_id = 1;
    std::unordered_map<std::string, FontFaceId> postscript_to_id;
    std::unordered_map<FontFaceId, ScopedCFTypeRef<CTFontDescriptorRef>> id_to_desc;
};

FontDatabase::FontDatabase() : impl_(std::make_unique<Impl>()) {}

FontDatabase::~FontDatabase() = default;

std::optional<FontFaceId> FontDatabase::match(const FontRequest& request) {
    auto attrs = ScopedCFTypeRef<CFMutableDictionaryRef>(CFDictionaryCreateMutable(
        kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));

    if (!request.family.empty()) {
        auto family = base::sys_utf8_to_cfstring_ref(request.family);
        CFDictionarySetValue(attrs.get(), kCTFontFamilyNameAttribute, family.get());
    }

    auto traits = ScopedCFTypeRef<CFMutableDictionaryRef>(CFDictionaryCreateMutable(
        kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));

    uint32_t sym_mask = 0;
    if (request.weight == Weight::Bold) {
        sym_mask |= kCTFontTraitBold;
    }
    if (request.slant == Slant::Italic) {
        sym_mask |= kCTFontTraitItalic;
    }
    if (sym_mask != 0) {
        auto sym = ScopedCFTypeRef<CFNumberRef>(
            CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &sym_mask));
        CFDictionarySetValue(traits.get(), kCTFontSymbolicTrait, sym.get());
        CFDictionarySetValue(attrs.get(), kCTFontTraitsAttribute, traits.get());
    }

    auto desc =
        ScopedCFTypeRef<CTFontDescriptorRef>(CTFontDescriptorCreateWithAttributes(attrs.get()));
    auto matches = ScopedCFTypeRef<CFArrayRef>(
        CTFontDescriptorCreateMatchingFontDescriptors(desc.get(), nullptr));
    if (!matches || CFArrayGetCount(matches.get()) == 0) return std::nullopt;

    auto matched_desc = (CTFontDescriptorRef)CFArrayGetValueAtIndex(matches.get(), 0);
    if (!matched_desc) return std::nullopt;

    auto keep = ScopedCFTypeRef<CTFontDescriptorRef>(matched_desc, OwnershipPolicy::kRetain);
    auto postscript_name = ScopedCFTypeRef<CFStringRef>(
        (CFStringRef)CTFontDescriptorCopyAttribute(keep.get(), kCTFontNameAttribute));
    std::string key = base::sys_cfstring_ref_to_utf8(postscript_name.get());

    auto it = impl_->postscript_to_id.find(key);
    if (it != impl_->postscript_to_id.end()) {
        return it->second;
    }

    const FontFaceId id = impl_->next_id++;
    impl_->postscript_to_id.emplace(key, id);
    impl_->id_to_desc.emplace(id, std::move(keep));
    return id;
}

struct FontHandle::Impl {
    ScopedCFTypeRef<CTFontRef> ctfont;
};

FontHandle::~FontHandle() = default;
FontHandle::FontHandle(FontHandle&& other) = default;
FontHandle& FontHandle::operator=(FontHandle&& other) = default;

bool FontHandle::valid() const { return impl_ && impl_->ctfont.get() != nullptr; }

std::optional<font::FontHandle> FontDatabase::create_font(FontFaceId face, double size_px) {
    auto it = impl_->id_to_desc.find(face);
    if (it == impl_->id_to_desc.end()) return std::nullopt;

    CTFontDescriptorRef desc = it->second.get();
    if (!desc) return std::nullopt;

    auto ct = ScopedCFTypeRef<CTFontRef>(CTFontCreateWithFontDescriptor(desc, size_px, nullptr));
    if (!ct) return std::nullopt;

    FontHandle out;
    out.impl_ = std::make_unique<FontHandle::Impl>();
    out.impl_->ctfont = std::move(ct);
    return out;
}

ShapedLine TextShaper::shape(const FontHandle& font, std::string_view utf8) const {
    CTFontRef ctfont = font.impl_->ctfont.get();

    const void* keys[] = {kCTFontAttributeName};
    const void* vals[] = {ctfont};
    auto attrs = ScopedCFTypeRef<CFDictionaryRef>(
        CFDictionaryCreate(kCFAllocatorDefault, keys, vals, 1, &kCFTypeDictionaryKeyCallBacks,
                           &kCFTypeDictionaryValueCallBacks));
    auto text = base::sys_utf8_to_cfstring_ref(utf8);
    auto as = ScopedCFTypeRef<CFAttributedStringRef>(
        CFAttributedStringCreate(kCFAllocatorDefault, text.get(), attrs.get()));
    auto line = ScopedCFTypeRef<CTLineRef>(CTLineCreateWithAttributedString(as.get()));

    CFArrayRef runs = CTLineGetGlyphRuns(line.get());
    CFIndex run_count = CFArrayGetCount(runs);

    std::vector<ShapedRun> shaped_runs;
    shaped_runs.reserve(run_count);
    base::UTF16ToUTF8IndicesMap indices_map;
    indices_map.set_utf8(utf8);

    for (CFIndex r = 0; r < run_count; r++) {
        CTRunRef run = (CTRunRef)CFArrayGetValueAtIndex(runs, r);
        CFIndex n = CTRunGetGlyphCount(run);
        if (n <= 0) continue;

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

        std::vector<GlyphPlacement> glyph_placements;
        glyph_placements.reserve(n);

        for (CFIndex i = 0; i < n; i++) {
            // TODO: Handle kCFNotFound case in TextShaper::shape().
            if (indices[i] == kCFNotFound) {
                spdlog::error("TODO: Handle kCFNotFound case in TextShaper::shape()");
                NOTREACHED();
            }

            size_t utf8_index = indices_map[indices[i]];
            glyph_placements.push_back({
                .glyph = glyphs[i],
                .x_advance = advances[i].width,
                .y_advance = advances[i].height,
                .x_offset = positions[i].x,
                .y_offset = positions[i].y,
                .cluster = utf8_index,
            });
        }

        FontHandle handle;
        handle.impl_ = std::make_unique<FontHandle::Impl>();
        handle.impl_->ctfont = ScopedCFTypeRef<CTFontRef>(run_font, OwnershipPolicy::kRetain);
        shaped_runs.emplace_back(std::move(handle), glyph_placements);
    }

    CGFloat ascent = 0, descent = 0, leading = 0;
    double line_width = CTLineGetTypographicBounds(line.get(), &ascent, &descent, &leading);
    return {
        .runs = std::move(shaped_runs),
        .width = line_width,
        .ascent = ascent,
        .descent = descent,
    };
}

GlyphBitmap GlyphRasterizer::rasterize(
    const FontHandle& font, GlyphId glyph, double sub_x, double sub_y, int scale) const {
    CTFontRef ctfont = font.impl_->ctfont.get();

    // Glyph bounds in glyph space (relative to glyph origin).
    CGGlyph glyphs = glyph;
    CGRect gb =
        CTFontGetBoundingRectsForGlyphs(ctfont, kCTFontOrientationDefault, &glyphs, nullptr, 1);

    // Compute destination pixel rect.
    int x0 = (int)std::floor(gb.origin.x * scale);
    int y0 = (int)std::floor(gb.origin.y * scale);
    int x1 = (int)std::ceil((gb.origin.x + gb.size.width) * scale);
    int y1 = (int)std::ceil((gb.origin.y + gb.size.height) * scale);

    // Outset by 1 device pixel.
    x0 -= 1;
    y0 -= 1;
    x1 += 1;
    y1 += 1;

    size_t w = std::max(0, x1 - x0);
    size_t h = std::max(0, y1 - y0);
    if (w == 0 || h == 0) return {};

    size_t bytes_per_pixel = 4;
    size_t bytes_per_row = w * bytes_per_pixel;
    std::vector<uint8_t> pixels(h * bytes_per_row);

    auto cs = ScopedCGColorSpace(CGColorSpaceCreateDeviceRGB());
    auto ctx = ScopedCGContext(
        CGBitmapContextCreate(pixels.data(), w, h, 8, bytes_per_row, cs.get(),
                              kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host));

    CGContextSetAllowsFontSubpixelQuantization(ctx.get(), false);
    CGContextSetShouldSubpixelQuantizeFonts(ctx.get(), false);
    CGContextSetAllowsFontSubpixelPositioning(ctx.get(), true);
    CGContextSetShouldSubpixelPositionFonts(ctx.get(), true);

    // Map glyph drawing into destination user space, but clipped to this bitmap.
    // device_px = user * scale, so set scale then translate by bitmap origin in user units.
    CGContextScaleCTM(ctx.get(), scale, scale);
    CGContextTranslateCTM(ctx.get(), -(CGFloat)x0 / scale, -(CGFloat)y0 / scale);

    // Draw black on white to create mask. (Special path exists to speed this up in CG.)
    CGContextSetGrayFillColor(ctx.get(), 0.0f, 1.0f);
    CGPoint pos = {(CGFloat)sub_x / scale, (CGFloat)sub_y / scale};
    CTFontDrawGlyphs(ctfont, &glyphs, &pos, 1, ctx.get());

    return {
        .width = w,
        .height = h,
        .bytes_per_pixel = bytes_per_pixel,
        .bearing_x = x0,
        .bearing_y = y0,
        .pixels = std::move(pixels),
    };
}

}  // namespace font
