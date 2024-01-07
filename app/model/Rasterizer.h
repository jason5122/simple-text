#pragma once

#import <Cocoa/Cocoa.h>
#import <vector>

class Rasterizer {
public:
    Rasterizer();

    CGGlyph get_glyph(NSString* characterString);
    std::vector<uint8_t> rasterize_glyph(CGGlyph glyph);
    bool is_colored_placeholder();
};
