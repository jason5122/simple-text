#import "Rasterizer.h"
#import "util/LogUtil.h"

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
        logDefault(@"WindowController", @"%@ %@", familyName, style);

        CTFontRef font = CTFontCreateWithFontDescriptor(descriptor, 16, NULL);
    }

    CTFontRef appleSymbolsFont = CTFontCreateWithName(CFSTR("Apple Symbols"), 16, NULL);
}

void Rasterizer::get_glyph(NSString* characterString) {
    CTFontRef menloFont = CTFontCreateWithName(CFSTR("Menlo"), 16, NULL);
    CTFontRef appleEmojiFont = CTFontCreateWithName(CFSTR("Apple Color Emoji"), 16, NULL);

    unichar characters[1];
    [characterString getCharacters:characters range:NSMakeRange(0, 1)];
    CGGlyph glyphs[1];
    if (CTFontGetGlyphsForCharacters(menloFont, characters, glyphs, 1)) {
        logDefault(@"WindowController", @"got glyphs! %d", glyphs[0]);

        CGRect bounds;
        CTFontGetBoundingRectsForGlyphs(menloFont, kCTFontOrientationDefault, glyphs, &bounds, 1);
        logDefault(@"WindowController", @"(%f, %f) %fx%f", bounds.origin.x, bounds.origin.y,
                   bounds.size.width, bounds.size.height);

        bool isColored = CTFontGetSymbolicTraits(appleEmojiFont) & kCTFontTraitColorGlyphs;
        if (isColored) {
            logDefault(@"WindowController", @"font is colored");
        }
    } else {
        logDefault(@"WindowController", @"could not get glyphs for characters");
    }
}
