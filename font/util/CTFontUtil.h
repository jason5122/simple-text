#pragma once

#import "util/log_util.h"
#import <Cocoa/Cocoa.h>
#import <iostream>
#import <vector>

struct CTRunResult {
    CGGlyph glyph;
    CTFontRef runFont;
};

static inline CTRunResult CTFontGetGlyphIndex(CTFontRef fontRef, const char* utf8_str) {
    size_t bytes = strlen(utf8_str);
    CFStringRef textString = CFStringCreateWithBytes(kCFAllocatorDefault, (const uint8_t*)utf8_str,
                                                     bytes, kCFStringEncodingUTF8, false);

    CFMutableDictionaryRef attr = CFDictionaryCreateMutable(
        kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionaryAddValue(attr, kCTFontAttributeName, fontRef);

    CFAttributedStringRef attrString =
        CFAttributedStringCreate(kCFAllocatorDefault, textString, attr);

    // CTTypesetterRef typesetter = CTTypesetterCreateWithAttributedString(attrString);
    // CTLineRef line = CTTypesetterCreateLine(typesetter, {0, 0});
    // This is a shorthand for accomplishing the above two lines.
    CTLineRef line = CTLineCreateWithAttributedString(attrString);

    CFArrayRef runArray = CTLineGetGlyphRuns(line);
    CFIndex runCount = CFArrayGetCount(runArray);
    for (CFIndex i = 0; i < runCount; i++) {
        CTRunRef run = (CTRunRef)CFArrayGetValueAtIndex(runArray, i);
        CTFontRef runFont =
            (CTFontRef)CFDictionaryGetValue(CTRunGetAttributes(run), kCTFontAttributeName);

        CFIndex runGlyphs = CTRunGetGlyphCount(run);

        std::vector<CGGlyph> glyphs(runGlyphs, 0);
        CTRunGetGlyphs(run, {0, runGlyphs}, &glyphs[0]);

        for (CGGlyph glyph : glyphs) {
            return CTRunResult{glyph, (CTFontRef)CFRetain(runFont)};
        }
    }
    return {0, fontRef};
}

static inline bool CTFontIsColored(CTFontRef fontRef) {
    return CTFontGetSymbolicTraits(fontRef) & kCTFontTraitColorGlyphs;
}

static inline bool CTFontIsMonospace(CTFontRef fontRef) {
    return CTFontGetSymbolicTraits(fontRef) & kCTFontTraitMonoSpace;
}
