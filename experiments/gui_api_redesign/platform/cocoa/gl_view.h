#pragma once

#include "experiments/gui_api_redesign/platform/cocoa/gl_context_manager.h"
#include "experiments/gui_api_redesign/window.h"
#include <AppKit/AppKit.h>

@interface GLView : NSView

- (instancetype)initWithFrame:(NSRect)frameRect
                    appWindow:(Window*)appWindow
             glContextManager:(GLContextManager*)mgr;

@end
