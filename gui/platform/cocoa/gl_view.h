#pragma once

#include "gui/platform/cocoa/display_gl.h"
#include "gui/platform/window_widget.h"

#include <Cocoa/Cocoa.h>

@interface GLView : NSView <NSTextInputClient>

- (instancetype)initWithFrame:(NSRect)frame
                    appWindow:(gui::WindowWidget*)appWindow
                    displayGL:(gui::DisplayGL*)displayGL;

- (void)redraw;
- (void)invalidateAppWindowPointer;
- (void)setAutoRedraw:(bool)autoRedraw;
- (int)framesPerSecond;

@end
