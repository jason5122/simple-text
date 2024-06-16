#pragma once

#include "app/cocoa/display_gl.h"
#include "app/window.h"
#import <QuartzCore/QuartzCore.h>

@interface GLLayer : CAOpenGLLayer {
@public
    app::Window* appWindow;
}

- (instancetype)initWithDisplayGL:(app::DisplayGL*)displayGL;

@end
