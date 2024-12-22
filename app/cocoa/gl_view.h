#pragma once

#include "app/cocoa/display_gl.h"
#include "app/window.h"

#include <Cocoa/Cocoa.h>

@interface GLView : NSView <NSTextInputClient>

- (instancetype)initWithFrame:(NSRect)frame
                    appWindow:(app::Window*)appWindow
                    displayGL:(app::DisplayGL*)displayGL;

- (void)redraw;

@end
