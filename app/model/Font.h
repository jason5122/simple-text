#pragma once

#import <Cocoa/Cocoa.h>

struct Metrics {
    double average_advance;
    double line_height;
};

class Font {
    CTFontRef fontRef;

public:
    Font(CFStringRef name, CGFloat size);
    Metrics metrics();
    CGGlyph get_glyph(NSString* characterString);
};
