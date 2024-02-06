#pragma once

#import "util/CGFloatUtil.h"
#import "util/log_util.h"
#import <Cocoa/Cocoa.h>

static inline CGGlyph CTFontGetGlyphIndex(CTFontRef fontRef, const char* utf8_str) {
    NSString* chString = [NSString stringWithUTF8String:utf8_str];
    UTF32Char outputChar;
    [chString getBytes:&outputChar
             maxLength:4
            usedLength:NULL
              encoding:NSUTF32LittleEndianStringEncoding
               options:0
                 range:NSMakeRange(0, chString.length)
        remainingRange:NULL];
    outputChar = NSSwapLittleIntToHost(outputChar);

    UniChar characters[2] = {};
    CFIndex length = CFStringGetSurrogatePairForLongCharacter(outputChar, characters) ? 2 : 1;

    CGGlyph glyphs[2] = {};
    CTFontGetGlyphsForCharacters(fontRef, characters, glyphs, length);
    return glyphs[0];
}

static inline bool CTFontIsColored(CTFontRef fontRef) {
    return CTFontGetSymbolicTraits(fontRef) & kCTFontTraitColorGlyphs;
}

static inline bool CTFontIsMonospace(CTFontRef fontRef) {
    return CTFontGetSymbolicTraits(fontRef) & kCTFontTraitMonoSpace;
}
