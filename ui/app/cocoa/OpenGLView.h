#include "ui/app/app_window.h"
#import <Cocoa/Cocoa.h>

@interface OpenGLView : NSView

- (instancetype)initWithFrame:(NSRect)frame appWindow:(AppWindow&)theAppWindow;

@end
