#include "ui/app/app.h"
#import <Cocoa/Cocoa.h>

@interface OpenGLView : NSView

- (instancetype)initWithFrame:(NSRect)frame appWindow:(App::Window*)theAppWindow;

- (void)redraw;

@end
