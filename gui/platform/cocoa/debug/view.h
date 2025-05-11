#pragma once

#include <Cocoa/Cocoa.h>

@interface View : NSView

- (instancetype)initWithFrame:(NSRect)frame;
- (void)redraw;
- (void)invalidateAppWindowPointer;
- (void)setAutoRedraw:(bool)autoRedraw;
- (int)framesPerSecond;

@end
