#include "ui/app/app.h"
#include "ui/app/cocoa/displaygl.h"
#import <Cocoa/Cocoa.h>

@interface WindowController : NSWindowController

- (instancetype)initWithFrame:(NSRect)frameRect
                    appWindow:(App::Window*)appWindow
                    displayGl:(DisplayGL*)displayGl;

- (void)show;
- (void)close;
- (void)redraw;

@end
