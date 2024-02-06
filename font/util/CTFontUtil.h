#pragma once

#import "util/CGFloatUtil.h"
#import "util/log_util.h"
#import <Cocoa/Cocoa.h>

static inline CGGlyph CTFontGetGlyphIndex(CTFontRef fontRef, const char* utf8_str) {
    NSString* chString = [NSString stringWithUTF8String:utf8_str];
    UniChar characters[1] = {};
    [chString getCharacters:characters range:NSMakeRange(0, 1)];

    CGGlyph glyphs[1] = {};
    CTFontGetGlyphsForCharacters(fontRef, characters, glyphs, 1);
    return glyphs[0];
}

static inline CGGlyph CTFontGetEmojiGlyphIndex(CTFontRef fontRef) {
    NSData* data = [@"\U0001F603" dataUsingEncoding:NSUTF32LittleEndianStringEncoding];
    UTF32Char emojiValue;
    [data getBytes:&emojiValue length:sizeof(emojiValue)];
    // Convert UTF32Char to UniChar surrogate pair.
    // Found here:
    // http://stackoverflow.com/questions/13005091/how-to-tell-if-a-particular-font-has-a-specific-glyph-64k#
    UniChar characters[2] = {};
    CFIndex length = (CFStringGetSurrogatePairForLongCharacter(emojiValue, characters) ? 2 : 1);

    CGGlyph glyphs[2] = {};
    CTFontGetGlyphsForCharacters(fontRef, characters, glyphs, length);
    return glyphs[0];
}

static inline CGGlyph CTFontGetEmojiGlyphIndex2(CTFontRef fontRef, const char* utf8_str) {
    NSString* chString = [NSString stringWithUTF8String:utf8_str];
    UTF32Char outputChar;
    [chString getBytes:&outputChar
             maxLength:4
            usedLength:NULL
              encoding:NSUTF32LittleEndianStringEncoding
               options:0
                 range:NSMakeRange(0, 2)
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
