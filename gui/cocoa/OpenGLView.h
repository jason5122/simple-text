#pragma once

#include "gui/cocoa/displaygl.h"
#include "gui/window.h"

#import <Cocoa/Cocoa.h>

@interface OpenGLView : NSView <NSTextInputClient>

- (instancetype)initWithFrame:(NSRect)frame
                    appWindow:(Window*)theAppWindow
                    displaygl:(DisplayGL*)displaygl;

- (void)redraw;

@end
