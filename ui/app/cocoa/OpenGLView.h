#include "ui/app/app.h"
#import <Cocoa/Cocoa.h>

@interface OpenGLView : NSView

- (instancetype)initWithFrame:(NSRect)frame window:(App::Window&)theWindow;

- (void)redraw;

@end
