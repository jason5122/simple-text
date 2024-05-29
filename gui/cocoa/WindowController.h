#pragma once

#include "gui/cocoa/displaygl.h"
#include "gui/window.h"
#import <Cocoa/Cocoa.h>

@interface WindowController : NSWindowController <NSWindowDelegate>

- (instancetype)initWithFrame:(NSRect)frameRect
                    appWindow:(Window*)appWindow
                    displayGl:(DisplayGL*)displayGl;

- (void)show;
- (void)close;
- (void)redraw;
- (int)getWidth;
- (int)getHeight;
- (int)getScaleFactor;
- (bool)isDarkMode;

@end
