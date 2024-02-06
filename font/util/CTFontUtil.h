#pragma once

#import "util/CGFloatUtil.h"
#import "util/log_util.h"
#import <Cocoa/Cocoa.h>
#import <iostream>
#import <vector>

// static inline CGGlyph CTFontGetGlyphIndex(CTFontRef fontRef, const char* utf8_str) {
//     NSString* chString = [NSString stringWithUTF8String:utf8_str];
//     NSData* unicharData = [chString dataUsingEncoding:NSUTF16LittleEndianStringEncoding];
//     const UniChar* characters = static_cast<const UniChar*>(unicharData.bytes);

//     CGGlyph glyphs[2] = {};
//     CTFontGetGlyphsForCharacters(fontRef, characters, glyphs, 2);
//     return glyphs[0];
// }

static inline CGGlyph CTFontGetGlyphIndex(CTFontRef fontRef, const char* utf8_str) {
    size_t bytes = strlen(utf8_str);
    CFStringRef textString = CFStringCreateWithBytes(kCFAllocatorDefault, (const uint8_t*)utf8_str,
                                                     bytes, kCFStringEncodingUTF8, false);

    CFMutableDictionaryRef attr = CFDictionaryCreateMutable(
        kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionaryAddValue(attr, kCTFontAttributeName, fontRef);

    CFAttributedStringRef attrString =
        CFAttributedStringCreate(kCFAllocatorDefault, textString, attr);

    CTTypesetterRef typesetter = CTTypesetterCreateWithAttributedString(attrString);

    CTLineRef line = CTTypesetterCreateLine(typesetter, {0, 0});

    CFArrayRef runArray = CTLineGetGlyphRuns(line);
    CFIndex runCount = CFArrayGetCount(runArray);
    for (CFIndex i = 0; i < runCount; i++) {
        CTRunRef run = (CTRunRef)CFArrayGetValueAtIndex(runArray, i);
        CFIndex runGlyphs = CTRunGetGlyphCount(run);

        std::vector<CGGlyph> glyphs(runGlyphs, 0);
        CTRunGetGlyphs(run, {0, runGlyphs}, &glyphs[0]);

        for (CGGlyph glyph : glyphs) {
            return glyph;
        }
    }
    return 0;
}

static inline bool CTFontIsColored(CTFontRef fontRef) {
    return CTFontGetSymbolicTraits(fontRef) & kCTFontTraitColorGlyphs;
}

static inline bool CTFontIsMonospace(CTFontRef fontRef) {
    return CTFontGetSymbolicTraits(fontRef) & kCTFontTraitMonoSpace;
}
