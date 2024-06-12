#pragma once

#include "gui/cocoa/display_gl.h"
#include "gui/window.h"
#import <QuartzCore/QuartzCore.h>

@interface GLLayer : CAOpenGLLayer {
@public
    gui::Window* appWindow;
}

- (instancetype)initWithDisplayGL:(gui::DisplayGL*)displayGL;

@end
