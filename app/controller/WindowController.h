#import <Cocoa/Cocoa.h>

@interface WindowController : NSWindowController {
    NSView* openGLView;
    CAOpenGLLayer* openGLLayer;
}

- (instancetype)initWithFrame:(NSRect)frameRect;

- (void)showWindow;

@end
