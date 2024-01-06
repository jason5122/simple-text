#import "WindowController.h"
#import "view/MyNSOpenGLView.h"
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

        openGLView = [[NSView alloc] initWithFrame:frameRect];
        openGLLayer = [OpenGLLayer layer];
        openGLView.layer = openGLLayer;
        openGLLayer.asynchronous = true;

        // self.window.contentView = openGLView;
        self.window.contentView = [[MyNSOpenGLView alloc] initWithFrame:frameRect];
    }
    return self;
}

- (void)showWindow {
    [self.window center];
    [self.window setFrameAutosaveName:@"glyph-atlas-cpp"];
    [self.window makeKeyAndOrderFront:nil];
}

@end
