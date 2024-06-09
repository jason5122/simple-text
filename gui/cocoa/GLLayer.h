#pragma once

#include "gui/window.h"
#import <QuartzCore/QuartzCore.h>

@interface GLLayer : CAOpenGLLayer {
@public
    gui::Window* appWindow;
}

- (instancetype)initWithContext:(CGLContextObj)displayContext;

@end
