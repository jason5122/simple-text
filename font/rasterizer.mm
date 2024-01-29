#import "rasterizer.h"
#import "util/CGFloatUtil.h"
#import "util/CTFontUtil.h"
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
    Metrics metrics;

    RasterizedGlyph rasterizeGlyph(CGGlyph glyph, CTFontRef fontRef);
};

void libgraphemeExample(const char* s) {
    size_t ret;

    LogDefault("libgrapheme", s);

    // Print each grapheme cluster with byte-length.
    LogDefault("libgrapheme", "grapheme clusters in NUL-delimited input:");
    for (size_t offset = 0; s[offset] != '\0'; offset += ret) {
        ret = grapheme_next_character_break_utf8(s + offset, SIZE_MAX);

        char* slice;
        asprintf(&slice, "%2zu bytes | %.*s\n", ret, (int)ret, s + offset);
        LogDefault("libgrapheme", slice);
    }

    // Do the same, but this time string is length-delimited.
    size_t len = 17;
    LogDefault("libgrapheme", "grapheme clusters in input delimited to %zu bytes:", len);
    for (size_t offset = 0; offset < len; offset += ret) {
        ret = grapheme_next_character_break_utf8(s + offset, len - offset);

        char* slice;
        asprintf(&slice, "%2zu bytes | %.*s\n", ret, (int)ret, s + offset);
        LogDefault("libgrapheme", slice);
    }

    // Print each codepoint.
    LogDefault("libgrapheme", "codepoints:");
    for (size_t offset = 0; s[offset] != '\0'; offset += ret) {
        ret = grapheme_decode_utf8(s + offset, SIZE_MAX, NULL);

        char* slice;
        asprintf(&slice, "%2zu bytes | %.*s\n", ret, (int)ret, s + offset);
        LogDefault("libgrapheme", slice);
    }
}

const char* hex(char c) {
    const char REF[] = "0123456789ABCDEF";
    static char output[3] = "XX";
    output[0] = REF[0x0f & c >> 4];
    output[1] = REF[0x0f & c];
    return output;
}

Rasterizer::Rasterizer(std::string main_font_name, std::string emoji_font_name, int font_size)
    : pimpl{new impl{}} {
    CFStringRef mainFontName =
        CFStringCreateWithCString(nullptr, main_font_name.c_str(), kCFStringEncodingUTF8);
    CFStringRef emojiFontName =
        CFStringCreateWithCString(nullptr, emoji_font_name.c_str(), kCFStringEncodingUTF8);

    pimpl->mainFont = CTFontCreateWithName(mainFontName, font_size, nullptr);
    pimpl->emojiFont = CTFontCreateWithName(emojiFontName, font_size, nullptr);
    pimpl->metrics = CTFontGetMetrics(pimpl->mainFont);

    if (CTFontIsMonospace(pimpl->mainFont)) {
        LogDefault(@"Rasterizer", @"Using monospace font.");
    }

    // UTF-8 encoded input
    const char* s = "T\xC3\xABst \xF0\x9F\x91\xA8\xE2\x80\x8D\xF0"
                    "\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA6 \xF0"
                    "\x9F\x87\xBA\xF0\x9F\x87\xB8 \xE0\xA4\xA8\xE0"
                    "\xA5\x80 \xE0\xAE\xA8\xE0\xAE\xBF!";
    libgraphemeExample(s);

    std::string str = u8"ABC가나다";
    std::cout << str << ": ";
    for (auto c : str) {
        std::cout << hex(c) << " ";
    }
    std::cout << '\n';

    NSString* chString = [NSString stringWithUTF8String:"\xC3\xAB"];
    UniChar characters[1] = {};
    [chString getCharacters:characters range:NSMakeRange(0, 1)];

    std::cout << hex(characters[0]) << '\n';
}

RasterizedGlyph Rasterizer::rasterizeChar(char ch, bool emoji) {
    CGGlyph glyph_index = emoji ? CTFontGetEmojiGlyphIndex(pimpl->emojiFont)
                                : CTFontGetGlyphIndex(pimpl->mainFont, ch);
    CTFontRef fontRef = emoji ? pimpl->emojiFont : pimpl->mainFont;
    return pimpl->rasterizeGlyph(glyph_index, fontRef);
}

bool Rasterizer::isFontMonospace() {
    return CTFontIsMonospace(pimpl->mainFont);
}

Metrics Rasterizer::metrics() {
    return pimpl->metrics;
}

RasterizedGlyph Rasterizer::impl::rasterizeGlyph(CGGlyph glyph_index, CTFontRef fontRef) {
    CGRect bounds;
    CTFontGetBoundingRectsForGlyphs(fontRef, kCTFontOrientationDefault, &glyph_index, &bounds, 1);
    // LogDefault(@"Rasterizer", @"(%f, %f) %fx%f", bounds.origin.x, bounds.origin.y,
    //            bounds.size.width, bounds.size.height);

    int32_t rasterizedLeft = CGFloat_floor(bounds.origin.x);
    uint32_t rasterizedWidth = CGFloat_ceil(bounds.origin.x - rasterizedLeft + bounds.size.width);
    int32_t rasterizedDescent = CGFloat_ceil(-bounds.origin.y);
    int32_t rasterizedAscent = CGFloat_ceil(bounds.size.height + bounds.origin.y);
    uint32_t rasterizedHeight = rasterizedDescent + rasterizedAscent;
    int32_t top = CGFloat_ceil(bounds.size.height + bounds.origin.y);

    top -= metrics.descent;

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
