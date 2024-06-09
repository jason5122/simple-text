#pragma once

#include "gui/cocoa/displaygl.h"
#include "gui/window.h"

#import <Cocoa/Cocoa.h>

@interface GLView : NSView <NSTextInputClient>

- (instancetype)initWithFrame:(NSRect)frame
                    appWindow:(gui::Window*)theAppWindow
                    displaygl:(gui::DisplayGL*)displaygl;

- (void)redraw;

@end
