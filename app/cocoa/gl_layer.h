#pragma once

#include "app/cocoa/display_gl.h"
#include "app/window.h"

#include <QuartzCore/QuartzCore.h>

@interface GLLayer : CAOpenGLLayer

- (instancetype)initWithAppWindow:(app::Window*)appWindow displayGL:(app::DisplayGL*)displayGL;

@end
