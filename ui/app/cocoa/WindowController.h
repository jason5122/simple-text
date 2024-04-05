#include "ui/app/cocoa/OpenGLView.h"
#import <Cocoa/Cocoa.h>

@interface WindowController : NSWindowController {
    OpenGLView* openGLView;
}

- (instancetype)initWithFrame:(NSRect)frameRect;

- (void)showWindow;

@end
