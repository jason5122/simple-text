#include "WindowController.h"
#include "ui/app/cocoa/OpenGLView.h"

@interface WindowController () {
    OpenGLView* opengl_view;
}
@end

@implementation WindowController

- (instancetype)initWithFrame:(NSRect)frameRect
                    appWindow:(App::Window*)appWindow
                    displayGl:(DisplayGL*)displayGl {
    self = [super init];
    if (self) {
        NSWindowStyleMask mask = NSWindowStyleMaskTitled | NSWindowStyleMaskResizable |
                                 NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
        self.window = [[[NSWindow alloc] initWithContentRect:frameRect
                                                   styleMask:mask
                                                     backing:NSBackingStoreBuffered
                                                       defer:false] autorelease];
        self.window.title = @"Simple Text";
        opengl_view = [[[OpenGLView alloc] initWithFrame:frameRect
                                               appWindow:appWindow
                                               displaygl:displayGl] autorelease];
        self.window.contentView = opengl_view;
        [self.window makeFirstResponder:opengl_view];

        // Bypass the user's tabbing preference.
        // https://stackoverflow.com/a/40826761/14698275
        self.window.tabbingMode = NSWindowTabbingModeDisallowed;
    }
    return self;
}

- (void)show {
    [self.window makeKeyAndOrderFront:nil];
}

- (void)close {
    [self.window close];
}

- (void)redraw {
    [opengl_view redraw];
}

@end
