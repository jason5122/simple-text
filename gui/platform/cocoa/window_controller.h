#pragma once

#include "gui/platform/cocoa/display_gl.h"
#include "gui/platform/window_widget.h"

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
- (int)getScaleFactor;
- (bool)isDarkMode;
- (void)setTitle:(std::string_view)title;
- (void)setFilePath:(std::string_view)path;
- (void)invalidateAppWindowPointer;
- (void)setAutoRedraw:(bool)autoRedraw;
- (int)framesPerSecond;

@end
