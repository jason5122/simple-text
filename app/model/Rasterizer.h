#pragma once

#import <Cocoa/Cocoa.h>
#import <vector>

struct RasterizedGlyph {
    char character;
    int32_t width;
    int32_t height;
    int32_t top;
    int32_t left;
    std::vector<uint8_t> buffer;
};

class Rasterizer {
public:
    Rasterizer();

    CGGlyph get_glyph(NSString* characterString);
    RasterizedGlyph rasterize_glyph(CGGlyph glyph);
    bool is_colored_placeholder();
};
