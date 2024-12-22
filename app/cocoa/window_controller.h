#pragma once

#include "app/cocoa/display_gl.h"
#include "app/window.h"
#include <string>

#include <Cocoa/Cocoa.h>

@interface WindowController : NSWindowController <NSWindowDelegate>

@property(nonatomic) std::weak_ptr<app::Window> appWindow;

- (instancetype)initWithFrame:(NSRect)frameRect
                    appWindow:(std::weak_ptr<app::Window>)appWindow
                    displayGL:(app::DisplayGL*)displayGL;

- (void)show;
- (void)close;
- (void)redraw;
- (int)getWidth;
- (int)getHeight;
- (int)getScaleFactor;
- (bool)isDarkMode;
- (void)setTitle:(std::string_view)title;
- (void)setFilePath:(std::string_view)path;

@end
