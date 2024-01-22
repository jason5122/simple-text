#pragma once

#import <Cocoa/Cocoa.h>
#import <vector>

struct RasterizedGlyph {
    bool colored;
    int32_t left;
    int32_t top;
    int32_t width;
    int32_t height;
    std::vector<uint8_t> buffer;
};

class Rasterizer {
public:
    Rasterizer();
    RasterizedGlyph rasterizeGlyph(CGGlyph glyph, CTFontRef fontRef);
};
