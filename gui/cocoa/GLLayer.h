#pragma once

#include "gui/cocoa/displaygl.h"
#include "gui/window.h"
#import <QuartzCore/QuartzCore.h>

@interface GLLayer : CAOpenGLLayer {
@public
    gui::Window* appWindow;

@private
    gui::DisplayGL* displaygl;
}

- (instancetype)initWithDisplayGL:(gui::DisplayGL*)theDisplaygl;

@end
