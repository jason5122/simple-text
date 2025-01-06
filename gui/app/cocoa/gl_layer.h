#pragma once

#include "gui/app/cocoa/display_gl.h"
#include "gui/app/window.h"

#include <QuartzCore/QuartzCore.h>

@interface GLLayer : CAOpenGLLayer

- (instancetype)initWithAppWindow:(gui::Window*)appWindow displayGL:(gui::DisplayGL*)displayGL;
- (void)invalidateAppWindowPointer;
- (void)setAutoRedraw:(bool)autoRedraw;
- (int)framesPerSecond;

@end
