#include "gui/app.h"
#include "gui/cocoa/displaygl.h"
#import <Cocoa/Cocoa.h>

@interface OpenGLView : NSView

- (instancetype)initWithFrame:(NSRect)frame
                    appWindow:(App::Window*)theAppWindow
                    displaygl:(DisplayGL*)displaygl;

- (void)redraw;

@end
