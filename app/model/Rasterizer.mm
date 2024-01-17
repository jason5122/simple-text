#import "Rasterizer.h"
#import "util/CGFloatUtil.h"
#import "util/LogUtil.h"
#import <Cocoa/Cocoa.h>

Rasterizer::Rasterizer() {
    NSDictionary* descriptorOptions = @{(id)kCTFontFamilyNameAttribute : @"Menlo"};
    CTFontDescriptorRef descriptor =
        CTFontDescriptorCreateWithAttributes((CFDictionaryRef)descriptorOptions);
    CFTypeRef keys[] = {kCTFontFamilyNameAttribute};
    CFSetRef mandatoryAttrs = CFSetCreate(kCFAllocatorDefault, keys, 1, &kCFTypeSetCallBacks);
    CFArrayRef fontDescriptors = CTFontDescriptorCreateMatchingFontDescriptors(descriptor, NULL);

    for (int i = 0; i < CFArrayGetCount(fontDescriptors); i++) {
        CTFontDescriptorRef descriptor =
            (CTFontDescriptorRef)CFArrayGetValueAtIndex(fontDescriptors, i);
        CFStringRef familyName =
            (CFStringRef)CTFontDescriptorCopyAttribute(descriptor, kCTFontFamilyNameAttribute);
        CFStringRef style =
            (CFStringRef)CTFontDescriptorCopyAttribute(descriptor, kCTFontStyleNameAttribute);
        logDefault(@"Rasterizer", @"%@ %@", familyName, style);

        CTFontRef font = CTFontCreateWithFontDescriptor(descriptor, 32, nullptr);
    }

    CTFontRef appleSymbolsFont = CTFontCreateWithName(CFSTR("Apple Symbols"), 32, nullptr);
}

RasterizedGlyph Rasterizer::rasterize_glyph(CGGlyph glyph) {
    CTFontRef menloFont = CTFontCreateWithName(CFSTR("Menlo"), 48, nullptr);

    CGRect bounds;
    CTFontGetBoundingRectsForGlyphs(menloFont, kCTFontOrientationDefault, &glyph, &bounds, 1);
    logDefault(@"Rasterizer", @"(%f, %f) %fx%f", bounds.origin.x, bounds.origin.y,
               bounds.size.width, bounds.size.height);

    int32_t rasterizedLeft = CGFloat_floor(bounds.origin.x);
    uint32_t rasterizedWidth = CGFloat_ceil(bounds.origin.x - rasterizedLeft + bounds.size.width);
    int32_t rasterizedDescent = CGFloat_ceil(-bounds.origin.y);
    int32_t rasterizedAscent = CGFloat_ceil(bounds.size.height + bounds.origin.y);
    uint32_t rasterizedHeight = rasterizedDescent + rasterizedAscent;

    CGContextRef context = CGBitmapContextCreate(
        nullptr, rasterizedWidth, rasterizedHeight, 8, rasterizedWidth * 4,
        CGColorSpaceCreateDeviceRGB(), kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host);
    CGContextSetRGBFillColor(context, 0.0, 0.0, 0.0, 1.0);

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

    CTFontDrawGlyphs(menloFont, &glyph, &rasterizationOrigin, 1, context);

    uint8_t* bitmapData = (uint8_t*)CGBitmapContextGetData(context);
    size_t height = CGBitmapContextGetHeight(context);
    size_t bytesPerRow = CGBitmapContextGetBytesPerRow(context);
    size_t len = height * bytesPerRow;

    CFStringRef fontFamily = CTFontCopyName(menloFont, kCTFontFamilyNameKey);
    CFStringRef fontFace = CTFontCopyName(menloFont, kCTFontSubFamilyNameKey);
    CGFloat fontSize = CTFontGetSize(menloFont);
    logDefault(@"Rasterizer", @"%@ %@ %f", fontFamily, fontFace, fontSize);
    logDefault(@"Rasterizer", @"%dx%d", rasterizedWidth, rasterizedHeight);
    logDefault(@"Rasterizer", @"height = %d, bytesPerRow = %d, len = %d", height, bytesPerRow,
               len);

    logDefault(@"Rasterizer", @"RGB = %d %d %d", bitmapData[2], bitmapData[1], bitmapData[0]);

    int pixels = len / 4;
    std::vector<uint8_t> rgb_buffer;
    // rgb_buffer.reserve(pixels * 3);
    rgb_buffer.reserve(pixels);
    for (int i = 0; i < pixels; i++) {
        int offset = i * 4;
        // rgb_buffer.push_back(bitmapData[offset + 2]);
        // rgb_buffer.push_back(bitmapData[offset + 1]);
        // rgb_buffer.push_back(bitmapData[offset]);

        uint8_t alpha = (bitmapData[offset] + bitmapData[offset + 1] + bitmapData[offset + 2]) / 3;
        rgb_buffer.push_back(alpha);

        logDefault(@"Rasterizer", @"%d", bitmapData[offset + 3]);
    }
    int32_t top = CGFloat_ceil(bounds.size.height + bounds.origin.y);
    return RasterizedGlyph{'E',
                           static_cast<int32_t>(rasterizedWidth),
                           static_cast<int32_t>(rasterizedHeight),
                           top,
                           rasterizedLeft,
                           rgb_buffer};
}

bool Rasterizer::is_colored_placeholder() {
    CTFontRef appleEmojiFont = CTFontCreateWithName(CFSTR("Apple Color Emoji"), 16, nullptr);
    bool isColored = CTFontGetSymbolicTraits(appleEmojiFont) & kCTFontTraitColorGlyphs;
    if (isColored) {
        logDefault(@"Rasterizer", @"font is colored");
    } else {
        logDefault(@"Rasterizer", @"font is NOT colored");
    }
    return isColored;
}
