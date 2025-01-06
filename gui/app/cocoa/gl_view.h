#pragma once

#include "gui/app/cocoa/display_gl.h"
#include "gui/app/window.h"

#include <Cocoa/Cocoa.h>

@interface GLView : NSView <NSTextInputClient>

- (instancetype)initWithFrame:(NSRect)frame
                    appWindow:(gui::Window*)appWindow
                    displayGL:(gui::DisplayGL*)displayGL;

- (void)redraw;
- (void)invalidateAppWindowPointer;
- (void)setAutoRedraw:(bool)autoRedraw;
- (int)framesPerSecond;

@end
