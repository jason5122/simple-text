#pragma once

#import <Cocoa/Cocoa.h>
#import <vector>

class RasterizedGlyph {
public:
    uint32_t width, height;
    std::vector<uint8_t> buffer;

    RasterizedGlyph(uint32_t width, uint32_t height, std::vector<uint8_t> buffer)
        : width(width), height(height), buffer(buffer){};
};

class Rasterizer {
public:
    Rasterizer();

    CGGlyph get_glyph(NSString* characterString);
    RasterizedGlyph rasterize_glyph(CGGlyph glyph);
    bool is_colored_placeholder();
};
