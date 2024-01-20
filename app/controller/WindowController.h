#import "view/OpenGLLayer.h"
#import <Cocoa/Cocoa.h>

@interface WindowController : NSWindowController {
    NSView* mainView;
    OpenGLLayer* openGLLayer;
}

- (instancetype)initWithFrame:(NSRect)frameRect;

- (void)showWindow;

@end
