#import "Rasterizer.h"
#import "util/LogUtil.h"

static inline CGFLOAT_TYPE CGFloat_floor(CGFLOAT_TYPE cgfloat) {
#if CGFLOAT_IS_DOUBLE
    return floor(cgfloat);
#else
    return floorf(cgfloat);
#endif
}
static inline CGFLOAT_TYPE CGFloat_ceil(CGFLOAT_TYPE cgfloat) {
#if CGFLOAT_IS_DOUBLE
    return ceil(cgfloat);
#else
    return ceilf(cgfloat);
#endif
}

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

        CTFontRef font = CTFontCreateWithFontDescriptor(descriptor, 16, NULL);
    }

    CTFontRef appleSymbolsFont = CTFontCreateWithName(CFSTR("Apple Symbols"), 16, NULL);
}

CGGlyph Rasterizer::get_glyph(NSString* characterString) {
    CTFontRef menloFont = CTFontCreateWithName(CFSTR("Menlo"), 16, NULL);

    unichar characters[1];
    [characterString getCharacters:characters range:NSMakeRange(0, 1)];
    CGGlyph glyphs[1];
    if (CTFontGetGlyphsForCharacters(menloFont, characters, glyphs, 1)) {
        logDefault(@"Rasterizer", @"got glyphs! %d", glyphs[0]);
    } else {
        logDefault(@"Rasterizer", @"could not get glyphs for characters");
    }
    return glyphs[0];
}

std::vector<uint8_t> Rasterizer::rasterize_glyph(CGGlyph glyph) {
    CTFontRef menloFont = CTFontCreateWithName(CFSTR("Menlo"), 16, NULL);

    CGRect bounds;
    CTFontGetBoundingRectsForGlyphs(menloFont, kCTFontOrientationDefault, &glyph, &bounds, 1);
    logDefault(@"Rasterizer", @"(%f, %f) %fx%f", bounds.origin.x, bounds.origin.y,
               bounds.size.width, bounds.size.height);

    CGFloat rasterizedLeft = CGFloat_floor(bounds.origin.x);
    CGFloat rasterizedWidth = CGFloat_ceil(bounds.origin.x - rasterizedLeft + bounds.size.width);
    CGFloat rasterizedDescent = CGFloat_ceil(-bounds.origin.y);
    CGFloat rasterizedAscent = CGFloat_ceil(bounds.size.height + bounds.origin.y);
    CGFloat rasterizedHeight = rasterizedDescent + rasterizedAscent;

    CGContextRef context = CGBitmapContextCreate(
        NULL, rasterizedWidth, rasterizedHeight, 8, rasterizedWidth * 4,
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
    size_t height = CGBitmapContextGetWidth(context);
    size_t bytesPerRow = CGBitmapContextGetBytesPerRow(context);
    size_t len = height * bytesPerRow;

    int pixels = len / 4;
    std::vector<uint8_t> rgb;
    rgb.reserve(pixels * 3);
    for (int i = 0; i < pixels; i++) {
        int offset = i * 4;
        rgb.push_back(bitmapData[offset + 2]);
        rgb.push_back(bitmapData[offset + 1]);
        rgb.push_back(bitmapData[offset]);
    }
    return rgb;
}

bool Rasterizer::is_colored_placeholder() {
    CTFontRef appleEmojiFont = CTFontCreateWithName(CFSTR("Apple Color Emoji"), 16, NULL);
    bool isColored = CTFontGetSymbolicTraits(appleEmojiFont) & kCTFontTraitColorGlyphs;
    if (isColored) {
        logDefault(@"Rasterizer", @"font is colored");
    } else {
        logDefault(@"Rasterizer", @"font is NOT colored");
    }
    return isColored;
}
