#pragma once

#include "gui/platform/cocoa/display_gl.h"
#include "gui/platform/window_widget.h"

#include <QuartzCore/QuartzCore.h>

@interface GLLayer : CAOpenGLLayer

- (instancetype)initWithAppWindow:(gui::WindowWidget*)appWindow
                        displayGL:(gui::DisplayGL*)displayGL;
- (void)invalidateAppWindowPointer;
- (void)setAutoRedraw:(bool)autoRedraw;
- (int)framesPerSecond;

@end
