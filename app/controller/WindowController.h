#import "view/OpenGLLayer.h"
#import <Cocoa/Cocoa.h>

@interface View : NSView
@end

@interface WindowController : NSWindowController {
    View* mainView;
    OpenGLLayer* openGLLayer;
}

- (instancetype)initWithFrame:(NSRect)frameRect;

- (void)showWindow;

@end
