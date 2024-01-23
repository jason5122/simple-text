#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>

@interface OpenGLLayer : CAOpenGLLayer {
@public
    float x, y;
    uint16_t row_offset;
    float y_accumulation;
}

- (void)insertCharacter:(char)ch;

@end
