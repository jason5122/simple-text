#pragma once

#include "gui/cocoa/displaygl.h"
#include "gui/window.h"
#include <string>

#import <Cocoa/Cocoa.h>

@interface WindowController : NSWindowController <NSWindowDelegate>

- (instancetype)initWithFrame:(NSRect)frameRect
                    appWindow:(gui::Window*)appWindow
                    displayGl:(gui::DisplayGL*)displayGl;

- (void)show;
- (void)close;
- (void)redraw;
- (int)getWidth;
- (int)getHeight;
- (int)getScaleFactor;
- (bool)isDarkMode;
- (void)setTitle:(const std::string&)title;
- (void)setFilePath:(fs::path)path;

@end
