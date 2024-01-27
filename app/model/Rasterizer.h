#pragma once

#import "app/model/RasterizedGlyph.h"
#import "app/util/CTFontUtil.h"
#import <Cocoa/Cocoa.h>

class Rasterizer {
public:
    Metrics metrics;

    Rasterizer(std::string main_font_name, std::string emoji_font_name, int font_size);
    RasterizedGlyph rasterizeChar(char ch, bool emoji);
    bool isFontMonospace();

private:
    CTFontRef mainFont;
    CTFontRef emojiFont;

    RasterizedGlyph rasterizeGlyph(CGGlyph glyph, CTFontRef fontRef);
};
