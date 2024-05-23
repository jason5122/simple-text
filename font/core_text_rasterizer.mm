#import <Cocoa/Cocoa.h>
#import <CoreText/CoreText.h>

#include "rasterizer.h"

namespace font {
class FontRasterizer::impl {
public:
    CTFontRef ct_font;

    RasterizedGlyph rasterizeGlyph(CGGlyph glyph, CTFontRef font_ref, float descent);
};

FontRasterizer::FontRasterizer() : pimpl{new impl{}} {}

FontRasterizer::~FontRasterizer() {
    CFRelease(pimpl->ct_font);
}

bool FontRasterizer::setup(int id, std::string main_font_name, int font_size) {
    this->id = id;

    CFStringRef ct_font_name =
        CFStringCreateWithCString(nullptr, main_font_name.c_str(), kCFStringEncodingUTF8);
    pimpl->ct_font = CTFontCreateWithName(ct_font_name, font_size, nullptr);

    CFRelease(ct_font_name);

    int ascent = std::ceil(CTFontGetAscent(pimpl->ct_font));
    int descent = std::ceil(CTFontGetDescent(pimpl->ct_font));
    int leading = std::ceil(CTFontGetLeading(pimpl->ct_font));
    int line_height = ascent + descent + leading;

    // TODO: Remove magic numbers that emulate Sublime Text.
    line_height += 1;

    this->line_height = line_height;
    this->descent = -descent;

    return true;
}

RasterizedGlyph FontRasterizer::rasterizeUTF8(std::string_view utf8_str) {
    CGGlyph glyph = 0;
    CTFontRef run_font = pimpl->ct_font;

    size_t num_bytes = utf8_str.size();
    CFStringRef text_string =
        CFStringCreateWithBytes(kCFAllocatorDefault, (const uint8_t*)&utf8_str[0], num_bytes,
                                kCFStringEncodingUTF8, false);

    CFMutableDictionaryRef attr = CFDictionaryCreateMutable(
        kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionaryAddValue(attr, kCTFontAttributeName, pimpl->ct_font);

    CFAttributedStringRef attr_string =
        CFAttributedStringCreate(kCFAllocatorDefault, text_string, attr);

    CTLineRef line = CTLineCreateWithAttributedString(attr_string);

    CFArrayRef run_array = CTLineGetGlyphRuns(line);
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
            CFRetain(font);
            run_font = font;
            break;
        }
    }

    CFRelease(text_string);
    CFRelease(attr);
    CFRelease(attr_string);
    CFRelease(line);

    return pimpl->rasterizeGlyph(glyph, run_font, descent);
}

RasterizedGlyph FontRasterizer::impl::rasterizeGlyph(CGGlyph glyph_index, CTFontRef font_ref,
                                                     float descent) {
    CGRect bounds;
    CTFontGetBoundingRectsForGlyphs(font_ref, kCTFontOrientationDefault, &glyph_index, &bounds, 1);

    int32_t rasterized_left = std::floor(bounds.origin.x);
    uint32_t rasterized_width = std::ceil(bounds.origin.x - rasterized_left + bounds.size.width);
    int32_t rasterized_descent = std::ceil(-bounds.origin.y);
    int32_t rasterized_ascent = std::ceil(bounds.size.height + bounds.origin.y);
    uint32_t rasterized_height = rasterized_descent + rasterized_ascent;

    int32_t top = std::ceil(bounds.size.height + bounds.origin.y);
    top -= descent;

    bool colored = CTFontGetSymbolicTraits(font_ref) & kCTFontTraitColorGlyphs;

    CGColorSpaceRef color_space_ref = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(
        nullptr, rasterized_width, rasterized_height, 8, rasterized_width * 4, color_space_ref,
        kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host);

    CGColorSpaceRelease(color_space_ref);

    CGFloat alpha = colored ? 0.0 : 1.0;
    CGContextSetRGBFillColor(context, 0.0, 0.0, 0.0, alpha);

    CGContextFillRect(context, CGRectMake(0.0, 0.0, rasterized_width, rasterized_height));
    CGContextSetAllowsFontSmoothing(context, true);
    CGContextSetShouldSmoothFonts(context, false);
    CGContextSetAllowsFontSubpixelQuantization(context, true);
    CGContextSetShouldSubpixelQuantizeFonts(context, true);
    CGContextSetAllowsFontSubpixelPositioning(context, true);
    CGContextSetShouldSubpixelPositionFonts(context, true);
    CGContextSetAllowsAntialiasing(context, true);
    CGContextSetShouldAntialias(context, true);

    CGContextSetRGBFillColor(context, 1.0, 1.0, 1.0, 1.0);
    CGPoint rasterization_origin = CGPointMake(-rasterized_left, rasterized_descent);

    CTFontDrawGlyphs(font_ref, &glyph_index, &rasterization_origin, 1, context);

    uint8_t* bitmap_data = (uint8_t*)CGBitmapContextGetData(context);
    size_t height = CGBitmapContextGetHeight(context);
    size_t bytes_per_row = CGBitmapContextGetBytesPerRow(context);
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

    CGContextRelease(context);

    return RasterizedGlyph{
        .colored = colored,
        .left = rasterized_left,
        .top = top,
        .width = static_cast<int32_t>(rasterized_width),
        .height = static_cast<int32_t>(rasterized_height),
        .advance = static_cast<int32_t>(std::ceil(advance)),
        .buffer = buffer,
        .index = glyph_index,
    };
}
}
