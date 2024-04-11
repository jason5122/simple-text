#include "ui/app/app.h"
#import <Cocoa/Cocoa.h>

@interface OpenGLView : NSView

- (instancetype)initWithFrame:(NSRect)frame appWindow:(Parent::Child*)theAppWindow;

- (void)redraw;

@end
