#include "view.h"

#include <fmt/base.h>

@implementation View

- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    return self;
}

- (void)drawRect:(NSRect)dirtyRect {
    fmt::println("NSView drawRect");
    [NSApp terminate:nil];
}

- (void)redraw {
}

- (void)invalidateAppWindowPointer {
}

- (void)setAutoRedraw:(bool)autoRedraw {
}

- (int)framesPerSecond {
    return 0;
}

@end
