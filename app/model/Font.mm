#import "Font.h"
#import "util/LogUtil.h"

Font::Font(CFStringRef name, CGFloat size) {
    fontRef = CTFontCreateWithName(name, size, NULL);
}

void Font::metrics() {}

CGGlyph Font::get_glyph(NSString* characterString) {
    unichar characters[1];
    [characterString getCharacters:characters range:NSMakeRange(0, 1)];
    CGGlyph glyphs[1];
    if (CTFontGetGlyphsForCharacters(fontRef, characters, glyphs, 1)) {
        logDefault(@"Font", @"got glyphs! %d", glyphs[0]);
    } else {
        logDefault(@"Font", @"could not get glyphs for characters");
    }
    return glyphs[0];
}
