#include "ui/app/app.h"
#include "ui/app/app_window.h"
#import <Cocoa/Cocoa.h>

@interface OpenGLView : NSView

- (instancetype)initWithFrame:(NSRect)frame appWindow:(AppWindow&)theAppWindow;

- (instancetype)initWithFrame:(NSRect)frame window:(App::Window&)theWindow;

- (instancetype)initWithFrame:(NSRect)frame;

@end
