#import "Rasterizer.h"
#import "util/CGFloatUtil.h"
#import "util/CTFontUtil.h"
#import "util/LogUtil.h"
#import <Cocoa/Cocoa.h>

Rasterizer::Rasterizer() {}

RasterizedGlyph Rasterizer::rasterizeGlyph(CGGlyph glyph, CTFontRef fontRef) {
    CGRect bounds;
    CTFontGetBoundingRectsForGlyphs(fontRef, kCTFontOrientationDefault, &glyph, &bounds, 1);
    // LogDefault(@"Rasterizer", @"(%f, %f) %fx%f", bounds.origin.x, bounds.origin.y,
    //            bounds.size.width, bounds.size.height);

    int32_t rasterizedLeft = CGFloat_floor(bounds.origin.x);
    uint32_t rasterizedWidth = CGFloat_ceil(bounds.origin.x - rasterizedLeft + bounds.size.width);
    int32_t rasterizedDescent = CGFloat_ceil(-bounds.origin.y);
    int32_t rasterizedAscent = CGFloat_ceil(bounds.size.height + bounds.origin.y);
    uint32_t rasterizedHeight = rasterizedDescent + rasterizedAscent;
    int32_t top = CGFloat_ceil(bounds.size.height + bounds.origin.y);

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

    CTFontDrawGlyphs(fontRef, &glyph, &rasterizationOrigin, 1, context);

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

    return RasterizedGlyph{
        colored,
        rasterizedLeft,
        top,
        static_cast<int32_t>(rasterizedWidth),
        static_cast<int32_t>(rasterizedHeight),
        buffer,
    };
}
