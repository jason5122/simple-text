#pragma once

#import "util/CGFloatUtil.h"
#import "util/log_util.h"
#import <Cocoa/Cocoa.h>
#import <iostream>
#import <vector>

static inline CGGlyph CTFontGetGlyphIndex(CTFontRef fontRef, const char* utf8_str) {
    NSString* chString = [NSString stringWithUTF8String:utf8_str];
    NSData* unicharData = [chString dataUsingEncoding:NSUTF16LittleEndianStringEncoding];
    const UniChar* characters = static_cast<const UniChar*>(unicharData.bytes);

    CGGlyph glyphs[2] = {};
    CTFontGetGlyphsForCharacters(fontRef, characters, glyphs, 2);
    return glyphs[0];

    // std::vector<UniChar> characters;
    // CFIndex total_length = 0;

    // std::vector<std::string> strs = {u8"🇺", u8"🇸"};
    // for (std::string str : strs) {
    //     NSString* chString = [NSString stringWithUTF8String:str.c_str()];
    //     UTF32Char outputChar;
    //     [chString getBytes:&outputChar
    //              maxLength:4
    //             usedLength:NULL
    //               encoding:NSUTF32LittleEndianStringEncoding
    //                options:0
    //                  range:NSMakeRange(0, chString.length)
    //         remainingRange:NULL];
    //     outputChar = NSSwapLittleIntToHost(outputChar);

    //     UniChar unichars[2] = {};
    //     CFIndex length = CFStringGetSurrogatePairForLongCharacter(outputChar, unichars) ? 2 : 1;

    //     for (UniChar ch : unichars) {
    //         characters.push_back(ch);
    //     }
    //     total_length += length;
    // }

    // for (UniChar ch : characters) {
    //     std::cout << ch << ' ';
    // }
    // std::cout << '\n';
    // std::cout << "total_length = " << total_length << '\n';

    // std::vector<UniChar> characters = {55357, 56396, 55356, 57342};
    // // std::vector<UniChar> characters = {0xD83C, 0xDDFA, 0xD83C, 0xDDF8};
    // CGGlyph glyphs[4] = {};
    // // CTFontGetGlyphsForCharacters(fontRef, &characters[0], glyphs, 4);
    // CTFontGetGlyphsForCharacters(fontRef, uchars, glyphs, 4);

    // for (CGGlyph glyph_index : glyphs) {
    //     std::cout << glyph_index << ' ';
    // }
    // std::cout << '\n';
    // return glyphs[0];
}

// static inline CGGlyph CTFontGetGlyphIndex(CTFontRef fontRef, const char* utf8_str) {
//     NSString* chString = [NSString stringWithUTF8String:utf8_str];
//     UTF32Char outputChar;
//     [chString getBytes:&outputChar
//              maxLength:4
//             usedLength:NULL
//               encoding:NSUTF32LittleEndianStringEncoding
//                options:0
//                  range:NSMakeRange(0, chString.length)
//         remainingRange:NULL];
//     outputChar = NSSwapLittleIntToHost(outputChar);

//     UniChar characters[2] = {};
//     CFIndex length = CFStringGetSurrogatePairForLongCharacter(outputChar, characters) ? 2 : 1;

//     std::cout << utf8_str << ' ' << outputChar << ' ' << length << '\n';

//     CGGlyph glyphs[2] = {};
//     CTFontGetGlyphsForCharacters(fontRef, characters, glyphs, length);
//     return glyphs[0];
// }

static inline bool CTFontIsColored(CTFontRef fontRef) {
    return CTFontGetSymbolicTraits(fontRef) & kCTFontTraitColorGlyphs;
}

static inline bool CTFontIsMonospace(CTFontRef fontRef) {
    return CTFontGetSymbolicTraits(fontRef) & kCTFontTraitMonoSpace;
}
