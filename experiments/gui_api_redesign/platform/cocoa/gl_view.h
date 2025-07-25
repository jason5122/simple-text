#pragma once

#include "experiments/gui_api_redesign/platform/cocoa/gl_context.h"
#include "experiments/gui_api_redesign/platform/cocoa/gl_pixel_format.h"
#include "experiments/gui_api_redesign/window.h"
#include <AppKit/AppKit.h>

@interface GLView : NSView

- (instancetype)initWithFrame:(NSRect)frameRect
                    appWindow:(Window*)appWindow
                    glContext:(GLContext*)ctx
                glPixelFormat:(GLPixelFormat*)pf;

@end
