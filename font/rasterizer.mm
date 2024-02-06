#import "font/util/CTFontUtil.h"
#import "rasterizer.h"
#import "util/CGFloatUtil.h"
#import "util/log_util.h"
#import <Cocoa/Cocoa.h>
#import <iostream>

extern "C" {
#include "third_party/libgrapheme/grapheme.h"
}

class Rasterizer::impl {
public:
    CTFontRef mainFont;
    CTFontRef emojiFont;

    RasterizedGlyph rasterizeGlyph(CGGlyph glyph, CTFontRef fontRef, float descent);
};

Rasterizer::Rasterizer(std::string main_font_name, std::string emoji_font_name, int font_size)
    : pimpl{new impl{}} {
    CFStringRef mainFontName =
        CFStringCreateWithCString(nullptr, main_font_name.c_str(), kCFStringEncodingUTF8);
    CFStringRef emojiFontName =
        CFStringCreateWithCString(nullptr, emoji_font_name.c_str(), kCFStringEncodingUTF8);

    pimpl->mainFont = CTFontCreateWithName(mainFontName, font_size, nullptr);
    pimpl->emojiFont = CTFontCreateWithName(emojiFontName, font_size, nullptr);

    // NSDictionary* descriptorOptions = @{(id)kCTFontFamilyNameAttribute : @"Source Code Pro"};
    // CTFontDescriptorRef descriptor =
    //     CTFontDescriptorCreateWithAttributes((CFDictionaryRef)descriptorOptions);
    // CFTypeRef keys[] = {kCTFontFamilyNameAttribute};
    // CFSetRef mandatoryAttrs = CFSetCreate(kCFAllocatorDefault, keys, 1, &kCFTypeSetCallBacks);
    // CFArrayRef fontDescriptors = CTFontDescriptorCreateMatchingFontDescriptors(descriptor,
    // NULL);

    // for (int i = 0; i < CFArrayGetCount(fontDescriptors); i++) {
    //     CTFontDescriptorRef descriptor =
    //         (CTFontDescriptorRef)CFArrayGetValueAtIndex(fontDescriptors, i);
    //     CFStringRef familyName =
    //         (CFStringRef)CTFontDescriptorCopyAttribute(descriptor, kCTFontFamilyNameAttribute);
    //     CFStringRef style =
    //         (CFStringRef)CTFontDescriptorCopyAttribute(descriptor, kCTFontStyleNameAttribute);

    //     if (CFEqual(style, CFSTR("Regular"))) {
    //         LogDefault("Renderer", "%@ %@", familyName, style);
    //         CTFontRef tempFont = CTFontCreateWithFontDescriptor(descriptor, font_size, nullptr);
    //         pimpl->mainFont = tempFont;
    //     }
    // }

    CGFloat ascent = CGFloat_round(CTFontGetAscent(pimpl->mainFont));
    CGFloat descent = CGFloat_round(CTFontGetDescent(pimpl->mainFont));
    CGFloat leading = CGFloat_round(CTFontGetLeading(pimpl->mainFont));
    CGFloat line_height = ascent + descent + leading;

    this->line_height = line_height + 2;
    this->descent = -descent;
}

RasterizedGlyph Rasterizer::rasterizeUTF8(const char* utf8_str) {
    CTRunResult runResult = CTFontGetGlyphIndex(pimpl->mainFont, utf8_str);
    return pimpl->rasterizeGlyph(runResult.glyph, runResult.runFont, descent);
}

bool Rasterizer::isFontMonospace() {
    return CTFontIsMonospace(pimpl->mainFont);
}

uint16_t Rasterizer::getGlyphIndex(const char* utf8_str) {
    CGGlyph glyph_index = CTFontGetGlyphIndex(pimpl->mainFont, utf8_str).glyph;
    if (glyph_index != 0) return glyph_index;
    return CTFontGetGlyphIndex(pimpl->emojiFont, utf8_str).glyph;
}

RasterizedGlyph Rasterizer::impl::rasterizeGlyph(CGGlyph glyph_index, CTFontRef fontRef,
                                                 float descent) {
    CGRect bounds;
    CTFontGetBoundingRectsForGlyphs(fontRef, kCTFontOrientationDefault, &glyph_index, &bounds, 1);
    LogDefault(@"Rasterizer", @"(%f, %f) %fx%f", bounds.origin.x, bounds.origin.y,
               bounds.size.width, bounds.size.height);

    int32_t rasterizedLeft = CGFloat_floor(bounds.origin.x);
    uint32_t rasterizedWidth = CGFloat_ceil(bounds.origin.x - rasterizedLeft + bounds.size.width);
    int32_t rasterizedDescent = CGFloat_ceil(-bounds.origin.y);
    int32_t rasterizedAscent = CGFloat_ceil(bounds.size.height + bounds.origin.y);
    uint32_t rasterizedHeight = rasterizedDescent + rasterizedAscent;
    int32_t top = CGFloat_ceil(bounds.size.height + bounds.origin.y);

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

    CFStringRef fontFamily = CTFontCopyName(fontRef, kCTFontFamilyNameKey);
    CFStringRef fontFace = CTFontCopyName(fontRef, kCTFontSubFamilyNameKey);
    CGFloat fontSize = CTFontGetSize(fontRef);

    // LogDefault(@"Rasterizer", @"%@ %@ %f", fontFamily, fontFace, fontSize);
    // LogDefault(@"Rasterizer", @"%dx%d", rasterizedWidth, rasterizedHeight);
    // LogDefault(@"Rasterizer", @"height = %d, bytesPerRow = %d, len = %d", height, bytesPerRow,
    //            len);
    // LogDefault(@"Rasterizer", @"RGB = %d %d %d", bitmapData[2], bitmapData[1], bitmapData[0]);

    int pixels = len / 4;
    std::vector<uint8_t> buffer;
    int size = colored ? pixels * 4 : pixels * 3;

    // TODO: This assumes little endian; detect and support big endian.
    buffer.reserve(size);
    for (int i = 0; i < pixels; i++) {
        int offset = i * 4;
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
        colored,
        rasterizedLeft,
        top,
        static_cast<int32_t>(rasterizedWidth),
        static_cast<int32_t>(rasterizedHeight),
        advance,
        buffer,
    };
}
