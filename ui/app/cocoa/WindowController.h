#include "ui/app/app_window.h"
#include "ui/app/cocoa/OpenGLView.h"
#import <Cocoa/Cocoa.h>

@interface WindowController : NSWindowController {
    OpenGLView* openGLView;
}

- (instancetype)initWithFrame:(NSRect)frameRect appWindow:(AppWindow&)theAppWindow;

- (void)showWindow;

@end
