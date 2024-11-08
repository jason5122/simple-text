#pragma once

#include "app/cocoa/display_gl.h"
#include "app/window.h"
#include <string>

#import <Cocoa/Cocoa.h>

@interface WindowController : NSWindowController <NSWindowDelegate>

- (instancetype)initWithFrame:(NSRect)frameRect
                    appWindow:(app::Window*)appWindow
                    displayGl:(app::DisplayGL*)displayGl;

- (void)show;
- (void)close;
- (void)redraw;
- (int)getWidth;
- (int)getHeight;
- (int)getScaleFactor;
- (bool)isDarkMode;
- (void)setTitle:(std::string_view)title;
- (void)setFilePath:(std::string_view)path;
- (app::Window*)getAppWindow;
- (NSWindow*)getNsWindow;

@end
