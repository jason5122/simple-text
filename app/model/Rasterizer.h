#pragma once

#import <Cocoa/Cocoa.h>
#import <vector>

struct RasterizedGlyph {
    // TODO: Reorder members to match AtlasGlyph member order.
    int32_t width;
    int32_t height;
    int32_t top;
    int32_t left;
    std::vector<uint8_t> buffer;
};

class Rasterizer {
public:
    Rasterizer(CTFontRef fontRef);
    RasterizedGlyph rasterizeGlyph(CGGlyph glyph);

private:
    CTFontRef fontRef;
};
