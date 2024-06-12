#pragma once

#include "gui/cocoa/display_gl.h"
#include "gui/window.h"

#import <Cocoa/Cocoa.h>

@interface GLView : NSView <NSTextInputClient>

- (instancetype)initWithFrame:(NSRect)frame
                    appWindow:(gui::Window*)appWindow
                    displaygl:(gui::DisplayGL*)displayGL;

- (void)redraw;

@end
