#import <Cocoa/Cocoa.h>

@interface OpenGLLayer : CAOpenGLLayer {
@public
    float x, y;
    CGPoint cursorPoint;
}

- (void)insertCharacter:(char)ch;

@end

@interface EditorView : NSView {
    NSTrackingArea* trackingArea;
}

@end
