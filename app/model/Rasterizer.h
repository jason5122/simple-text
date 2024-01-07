#pragma once

#import <Cocoa/Cocoa.h>

class Rasterizer {
public:
    Rasterizer();

    void get_glyph(NSString* characterString);
};
