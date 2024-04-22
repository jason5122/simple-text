#include "ui/app/app.h"
#include "ui/app/cocoa/displaygl.h"
#import <Cocoa/Cocoa.h>

@interface WindowController : NSWindowController <NSWindowDelegate>

- (instancetype)initWithFrame:(NSRect)frameRect
                    appWindow:(App::Window*)appWindow
                    displayGl:(DisplayGL*)displayGl;

- (void)show;
- (void)close;
- (void)redraw;
- (int)getWidth;
- (int)getHeight;
- (int)getScaleFactor;

@end
