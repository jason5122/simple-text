#pragma once

#include "gui/app/cocoa/display_gl.h"
#include "gui/app/window_widget.h"

#include <string>

#include <Cocoa/Cocoa.h>

@interface WindowController : NSWindowController <NSWindowDelegate>

@property(nonatomic) gui::WindowWidget* appWindow;

- (instancetype)initWithFrame:(NSRect)frameRect
                    appWindow:(gui::WindowWidget*)appWindow
                    displayGL:(gui::DisplayGL*)displayGL;

- (void)show;
- (void)close;
- (void)redraw;
- (int)getWidth;
- (int)getHeight;
- (int)getScaleFactor;
- (bool)isDarkMode;
- (void)setTitle:(std::string_view)title;
- (void)setFilePath:(std::string_view)path;
- (void)invalidateAppWindowPointer;
- (void)setAutoRedraw:(bool)autoRedraw;
- (int)framesPerSecond;

@end
