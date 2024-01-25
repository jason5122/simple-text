#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>

@interface OpenGLLayer : CAOpenGLLayer {
@public
    float x, y;
    CGPoint cursorPoint;
}

- (void)insertCharacter:(char)ch;

@end
