#import "WindowController.h"
#import "view/OpenGLLayer.h"

@implementation WindowController

- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super init];
    if (self) {
        unsigned int mask = NSWindowStyleMaskTitled | NSWindowStyleMaskResizable |
                            NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
        self.window = [[NSWindow alloc] initWithContentRect:frameRect
                                                  styleMask:mask
                                                    backing:NSBackingStoreBuffered
                                                      defer:false];
        self.window.title = @"Glyph Atlas C++";

        mainView = [[NSView alloc] initWithFrame:frameRect];
        mainView.layer = [OpenGLLayer layer];
        mainView.layer.needsDisplayOnBoundsChange = true;
        // openGLLayer.asynchronous = true;

        self.window.contentView = mainView;
    }
    return self;
}

- (void)showWindow {
    [self.window center];
    [self.window setFrameAutosaveName:@"glyph-atlas-cpp"];
    [self.window makeKeyAndOrderFront:nil];

    // OpenGLLayer does not display initially due to OpenGL uniform issue; use hack to display it
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_MSEC), dispatch_get_main_queue(),
                   ^{ [mainView.layer setNeedsDisplay]; });
}

@end
