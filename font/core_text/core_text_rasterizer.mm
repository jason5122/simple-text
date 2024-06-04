#include "base/apple/scoped_cftyperef.h"
#include "base/apple/scoped_cgtyperef.h"
#include "font/rasterizer.h"

#import <Cocoa/Cocoa.h>
#import <CoreText/CoreText.h>

using base::apple::ScopedCFTypeRef;
using base::apple::ScopedTypeRef;

namespace font {

class FontRasterizer::impl {
public:
    ScopedCFTypeRef<CTFontRef> ct_font;

    RasterizedGlyph rasterizeGlyph(CGGlyph glyph, ScopedCFTypeRef<CTFontRef> selected_font,
                                   int descent);
};

FontRasterizer::FontRasterizer(const std::string& font_name_utf8, int font_size)
    : pimpl{new impl{}} {
    ScopedCFTypeRef<CFStringRef> ct_font_name{
        CFStringCreateWithCString(nullptr, font_name_utf8.c_str(), kCFStringEncodingUTF8)};
    pimpl->ct_font.reset(CTFontCreateWithName(ct_font_name.get(), font_size, nullptr));

    int ascent = std::ceil(CTFontGetAscent(pimpl->ct_font.get()));
    int descent = std::ceil(CTFontGetDescent(pimpl->ct_font.get()));
    int leading = std::ceil(CTFontGetLeading(pimpl->ct_font.get()));
    int line_height = ascent + descent + leading;

    // TODO: Remove magic numbers that emulate Sublime Text.
    line_height += 1;

    this->line_height = line_height;
    this->descent = -descent;
}

FontRasterizer::~FontRasterizer() {}

RasterizedGlyph FontRasterizer::rasterizeUTF8(std::string_view str8) {
    CGGlyph glyph = 0;
    ScopedCFTypeRef<CTFontRef> run_font;

    ScopedCFTypeRef<CFStringRef> text_string{
        CFStringCreateWithBytes(kCFAllocatorDefault, (const uint8_t*)&str8[0], str8.length(),
                                kCFStringEncodingUTF8, false)};

    ScopedCFTypeRef<CFMutableDictionaryRef> attr{CFDictionaryCreateMutable(
        kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks)};
    CFDictionaryAddValue(attr.get(), kCTFontAttributeName, pimpl->ct_font.get());

    ScopedCFTypeRef<CFAttributedStringRef> attr_string{
        CFAttributedStringCreate(kCFAllocatorDefault, text_string.get(), attr.get())};

    ScopedCFTypeRef<CTLineRef> line{CTLineCreateWithAttributedString(attr_string.get())};

    CFArrayRef run_array = CTLineGetGlyphRuns(line.get());
    CFIndex run_count = CFArrayGetCount(run_array);
    for (CFIndex i = 0; i < run_count; i++) {
        CTRunRef run = (CTRunRef)CFArrayGetValueAtIndex(run_array, i);
        CTFontRef font =
            (CTFontRef)CFDictionaryGetValue(CTRunGetAttributes(run), kCTFontAttributeName);

        CFIndex run_glyphs = CTRunGetGlyphCount(run);

        std::vector<CGGlyph> glyphs(run_glyphs, 0);
        CTRunGetGlyphs(run, {0, run_glyphs}, &glyphs[0]);

        if (!glyphs.empty()) {
            glyph = glyphs[0];
            run_font.reset(font);
            break;
        }
    }

    return pimpl->rasterizeGlyph(glyph, run_font, descent);
}

RasterizedGlyph FontRasterizer::impl::rasterizeGlyph(CGGlyph glyph_index,
                                                     ScopedCFTypeRef<CTFontRef> selected_font,
                                                     int descent) {
    CTFontRef font_ref = selected_font ? selected_font.get() : ct_font.get();

    CGRect bounds;
    CTFontGetBoundingRectsForGlyphs(font_ref, kCTFontOrientationDefault, &glyph_index, &bounds, 1);

    int32_t rasterized_left = std::floor(bounds.origin.x);
    uint32_t rasterized_width = std::ceil(bounds.origin.x - rasterized_left + bounds.size.width);
    int32_t rasterized_descent = std::ceil(-bounds.origin.y);
    int32_t rasterized_ascent = std::ceil(bounds.size.height + bounds.origin.y);
    uint32_t rasterized_height = rasterized_descent + rasterized_ascent;

    int32_t top = std::ceil(bounds.size.height + bounds.origin.y);
    top -= descent;

    // If the font is a color font and the glyph doesn't have an outline, it is a color glyph.
    // https://github.com/sublimehq/sublime_text/issues/3747#issuecomment-726837744
    bool colored_font = CTFontGetSymbolicTraits(font_ref) & kCTFontTraitColorGlyphs;
    bool has_outline = CTFontCreatePathForGlyph(font_ref, glyph_index, nullptr);
    bool colored = colored_font && !has_outline;

    ScopedTypeRef<CGColorSpaceRef> color_space_ref{CGColorSpaceCreateDeviceRGB()};
    ScopedTypeRef<CGContextRef> context{CGBitmapContextCreate(
        nullptr, rasterized_width, rasterized_height, 8, rasterized_width * 4,
        color_space_ref.get(), kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host)};

    CGFloat alpha = colored ? 0.0 : 1.0;
    CGContextSetRGBFillColor(context.get(), 0.0, 0.0, 0.0, alpha);

    CGContextFillRect(context.get(), CGRectMake(0.0, 0.0, rasterized_width, rasterized_height));
    CGContextSetAllowsFontSmoothing(context.get(), true);
    CGContextSetShouldSmoothFonts(context.get(), false);
    CGContextSetAllowsFontSubpixelQuantization(context.get(), true);
    CGContextSetShouldSubpixelQuantizeFonts(context.get(), true);
    CGContextSetAllowsFontSubpixelPositioning(context.get(), true);
    CGContextSetShouldSubpixelPositionFonts(context.get(), true);
    CGContextSetAllowsAntialiasing(context.get(), true);
    CGContextSetShouldAntialias(context.get(), true);

    CGContextSetRGBFillColor(context.get(), 1.0, 1.0, 1.0, 1.0);
    CGPoint rasterization_origin = CGPointMake(-rasterized_left, rasterized_descent);

    CTFontDrawGlyphs(font_ref, &glyph_index, &rasterization_origin, 1, context.get());

    uint8_t* bitmap_data = (uint8_t*)CGBitmapContextGetData(context.get());
    size_t height = CGBitmapContextGetHeight(context.get());
    size_t bytes_per_row = CGBitmapContextGetBytesPerRow(context.get());
    size_t len = height * bytes_per_row;

    size_t pixels = len / 4;
    std::vector<uint8_t> buffer;
    size_t size = colored ? pixels * 4 : pixels * 3;

    // TODO: This assumes little endian; detect and support big endian.
    buffer.reserve(size);
    for (size_t i = 0; i < pixels; i++) {
        size_t offset = i * 4;
        buffer.emplace_back(bitmap_data[offset + 2]);
        buffer.emplace_back(bitmap_data[offset + 1]);
        buffer.emplace_back(bitmap_data[offset]);
        if (colored) {
            buffer.emplace_back(bitmap_data[offset + 3]);
        }
    }

    float advance =
        CTFontGetAdvancesForGlyphs(font_ref, kCTFontOrientationDefault, &glyph_index, nullptr, 1);

    return RasterizedGlyph{
        .colored = colored,
        .left = rasterized_left,
        .top = top,
        .width = static_cast<int32_t>(rasterized_width),
        .height = static_cast<int32_t>(rasterized_height),
        .advance = static_cast<int32_t>(std::ceil(advance)),
        .buffer = buffer,
    };
}

}
