#pragma once

#import "util/CGFloatUtil.h"
#import "util/LogUtil.h"

struct Metrics {
    double average_advance;
    double line_height;
};

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

static inline Metrics CTFontGetMetrics(CTFontRef fontRef) {
    CGGlyph glyph = CTFontGetGlyphIndex(fontRef, '0');
    double average_advance =
        CTFontGetAdvancesForGlyphs(fontRef, kCTFontOrientationDefault, &glyph, nullptr, 1);

    CGFloat ascent = CGFloat_round(CTFontGetAscent(fontRef));
    CGFloat descent = CGFloat_round(CTFontGetDescent(fontRef));
    CGFloat leading = CGFloat_round(CTFontGetLeading(fontRef));
    CGFloat line_height = ascent + descent + leading;

    return Metrics{average_advance, line_height};
}

static inline bool CTFontIsColored(CTFontRef fontRef) {
    return CTFontGetSymbolicTraits(fontRef) & kCTFontTraitColorGlyphs;
}
