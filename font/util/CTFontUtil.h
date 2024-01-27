#pragma once

#import "app/util/CGFloatUtil.h"
#import "app/util/LogUtil.h"
#import "font/types/Metrics.h"
#import <Cocoa/Cocoa.h>

static inline CGGlyph CTFontGetGlyphIndex(CTFontRef fontRef, char ch) {
    NSString* chString = [NSString stringWithFormat:@"%c", ch];
    unichar characters[1];
    [chString getCharacters:characters range:NSMakeRange(0, 1)];
    CGGlyph glyphs[1];
    if (CTFontGetGlyphsForCharacters(fontRef, characters, glyphs, 1)) {
        // LogDefault(@"CTFontUtil", @"got glyphs! %d", glyphs[0]);
    } else {
        LogDefault(@"CTFontUtil", @"could not get glyphs for char: %c, value: %d", ch, ch);
    }
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
    bool ret = CTFontGetGlyphsForCharacters(fontRef, characters, glyphs, length);
    if (ret) {
        LogDefault(@"Renderer", @"yo?????? ¬åß˚∆∂ƒˆøß∆∂");
    } else {
        LogDefault(@"Renderer", @"aw damn...");
    }
    return glyphs[0];
}

static inline Metrics CTFontGetMetrics(CTFontRef fontRef) {
    CGGlyph glyph = CTFontGetGlyphIndex(fontRef, '0');
    double average_advance =
        CTFontGetAdvancesForGlyphs(fontRef, kCTFontOrientationDefault, &glyph, nullptr, 1);

    CGFloat ascent = CGFloat_round(CTFontGetAscent(fontRef));
    CGFloat descent = CGFloat_round(CTFontGetDescent(fontRef));
    CGFloat leading = CGFloat_round(CTFontGetLeading(fontRef));
    CGFloat line_height = ascent + descent + leading;

    return Metrics{average_advance, line_height, -descent};
}

static inline bool CTFontIsColored(CTFontRef fontRef) {
    return CTFontGetSymbolicTraits(fontRef) & kCTFontTraitColorGlyphs;
}

static inline bool CTFontIsMonospace(CTFontRef fontRef) {
    return CTFontGetSymbolicTraits(fontRef) & kCTFontTraitMonoSpace;
}
