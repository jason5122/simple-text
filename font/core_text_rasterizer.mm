#include "font/rasterized_glyph.h"
#import "font/util/CTFontUtil.h"
#import "rasterizer.h"
#import <Cocoa/Cocoa.h>
#include <CoreText/CoreText.h>
#import <iostream>

class FontRasterizer::impl {
public:
    CTFontRef mainFont;

    RasterizedGlyph rasterizeGlyph(CGGlyph glyph, CTFontRef fontRef, float descent);
};

FontRasterizer::FontRasterizer() : pimpl{new impl{}} {}

bool FontRasterizer::setup(int id, std::string main_font_name, int font_size) {
    this->id = id;

    CFStringRef mainFontName =
        CFStringCreateWithCString(nullptr, main_font_name.c_str(), kCFStringEncodingUTF8);

    pimpl->mainFont = CTFontCreateWithName(mainFontName, font_size, nullptr);

    CGFloat ascent = std::round(CTFontGetAscent(pimpl->mainFont));
    CGFloat descent = std::round(CTFontGetDescent(pimpl->mainFont));
    CGFloat leading = std::round(CTFontGetLeading(pimpl->mainFont));
    CGFloat line_height = ascent + descent + leading;

    this->line_height = line_height + 2;
    this->descent = -descent;

    return true;
}

RasterizedGlyph FontRasterizer::rasterizeUTF8(const char* utf8_str) {
    CTRunResult runResult = CTFontGetGlyphIndex(pimpl->mainFont, utf8_str);
    return pimpl->rasterizeGlyph(runResult.glyph, runResult.runFont, descent);
}

std::vector<RasterizedGlyph> FontRasterizer::layoutLine(const char* utf8_str) {
    CTFontRef fontRef = pimpl->mainFont;

    size_t bytes = strlen(utf8_str);
    CFStringRef textString = CFStringCreateWithBytes(kCFAllocatorDefault, (const uint8_t*)utf8_str,
                                                     bytes, kCFStringEncodingUTF8, false);

    CFMutableDictionaryRef attr = CFDictionaryCreateMutable(
        kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionaryAddValue(attr, kCTFontAttributeName, fontRef);
    // CFDictionaryAddValue(attr, kCTLigatureAttributeName, (CFNumberRef) @0);

    CFAttributedStringRef attrString =
        CFAttributedStringCreate(kCFAllocatorDefault, textString, attr);

    CTLineRef line = CTLineCreateWithAttributedString(attrString);

    std::vector<RasterizedGlyph> rasterized_glyphs;

    CFArrayRef runArray = CTLineGetGlyphRuns(line);
    CFIndex runCount = CFArrayGetCount(runArray);
    for (CFIndex i = 0; i < runCount; i++) {
        CTRunRef run = (CTRunRef)CFArrayGetValueAtIndex(runArray, i);
        CTFontRef runFont =
            (CTFontRef)CFDictionaryGetValue(CTRunGetAttributes(run), kCTFontAttributeName);

        CFIndex count = CTRunGetGlyphCount(run);
        std::cerr << count << " glyphs in run" << '\n';

        std::vector<CGGlyph> glyphs(count, 0);
        CTRunGetGlyphs(run, {0, count}, &glyphs[0]);
        for (CGGlyph glyph : glyphs) {
            NSString* runFontName = (__bridge NSString*)CTFontCopyDisplayName(runFont);
            fprintf(stderr, "glyph id: %d, %s\n", glyph, runFontName.UTF8String);

            rasterized_glyphs.emplace_back(pimpl->rasterizeGlyph(glyph, runFont, descent));
        }

        std::vector<CGPoint> positions(count);
        CTRunGetPositions(run, {0, count}, &positions[0]);
        for (CGPoint& position : positions) {
            fprintf(stderr, "%f, %f\n", position.x, position.y);
        }
    }
    return rasterized_glyphs;
}

RasterizedGlyph FontRasterizer::impl::rasterizeGlyph(CGGlyph glyph_index, CTFontRef fontRef,
                                                     float descent) {
    CGRect bounds;
    CTFontGetBoundingRectsForGlyphs(fontRef, kCTFontOrientationDefault, &glyph_index, &bounds, 1);

    int32_t rasterizedLeft = std::floor(bounds.origin.x);
    uint32_t rasterizedWidth = std::ceil(bounds.origin.x - rasterizedLeft + bounds.size.width);
    int32_t rasterizedDescent = std::ceil(-bounds.origin.y);
    int32_t rasterizedAscent = std::ceil(bounds.size.height + bounds.origin.y);
    uint32_t rasterizedHeight = rasterizedDescent + rasterizedAscent;
    int32_t top = std::ceil(bounds.size.height + bounds.origin.y);

    // TODO: Apply this transformation in glyph atlas, not in rasterizer.
    top -= descent;

    bool colored = CTFontIsColored(fontRef);

    CGContextRef context = CGBitmapContextCreate(
        nullptr, rasterizedWidth, rasterizedHeight, 8, rasterizedWidth * 4,
        CGColorSpaceCreateDeviceRGB(), kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host);

    CGFloat alpha = colored ? 0.0 : 1.0;
    CGContextSetRGBFillColor(context, 0.0, 0.0, 0.0, alpha);

    CGContextFillRect(context, CGRectMake(0.0, 0.0, rasterizedWidth, rasterizedHeight));
    CGContextSetAllowsFontSmoothing(context, true);
    CGContextSetShouldSmoothFonts(context, false);
    CGContextSetAllowsFontSubpixelQuantization(context, true);
    CGContextSetShouldSubpixelQuantizeFonts(context, true);
    CGContextSetAllowsFontSubpixelPositioning(context, true);
    CGContextSetShouldSubpixelPositionFonts(context, true);
    CGContextSetAllowsAntialiasing(context, true);
    CGContextSetShouldAntialias(context, true);

    CGContextSetRGBFillColor(context, 1.0, 1.0, 1.0, 1.0);
    CGPoint rasterizationOrigin = CGPointMake(-rasterizedLeft, rasterizedDescent);

    CTFontDrawGlyphs(fontRef, &glyph_index, &rasterizationOrigin, 1, context);

    uint8_t* bitmapData = (uint8_t*)CGBitmapContextGetData(context);
    size_t height = CGBitmapContextGetHeight(context);
    size_t bytesPerRow = CGBitmapContextGetBytesPerRow(context);
    size_t len = height * bytesPerRow;

    size_t pixels = len / 4;
    std::vector<uint8_t> buffer;
    size_t size = colored ? pixels * 4 : pixels * 3;

    // TODO: This assumes little endian; detect and support big endian.
    buffer.reserve(size);
    for (size_t i = 0; i < pixels; i++) {
        size_t offset = i * 4;
        buffer.push_back(bitmapData[offset + 2]);
        buffer.push_back(bitmapData[offset + 1]);
        buffer.push_back(bitmapData[offset]);
        if (colored) buffer.push_back(bitmapData[offset + 3]);
    }

    // NOTE: Don't round here! Round after summing the total advance, right before passing to the
    // shader.
    float advance =
        CTFontGetAdvancesForGlyphs(fontRef, kCTFontOrientationDefault, &glyph_index, nullptr, 1);

    return RasterizedGlyph{
        .colored = colored,
        .left = rasterizedLeft,
        .top = top,
        .width = static_cast<int32_t>(rasterizedWidth),
        .height = static_cast<int32_t>(rasterizedHeight),
        .advance = advance,
        .buffer = buffer,
        .index = glyph_index,
    };
}

FontRasterizer::~FontRasterizer() {}
