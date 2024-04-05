#include "ui/app/app.h"
#include "ui/app/cocoa/OpenGLView.h"
#import <Cocoa/Cocoa.h>

@interface WindowController : NSWindowController {
    OpenGLView* openGLView;
}

- (instancetype)initWithFrame:(NSRect)frameRect app:(App*)app;

- (void)showWindow;

@end
