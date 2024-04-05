#include "ui/app/app.h"
#import <Cocoa/Cocoa.h>

@interface OpenGLView : NSView {
    App* app;
}

- (instancetype)initWithFrame:(NSRect)frame app:(App*)theApp;

@end
