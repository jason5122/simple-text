#include "WindowController.h"
#include <AppKit/AppKit.h>
#include <iostream>

@implementation WindowController

- (instancetype)initWithFrame:(NSRect)frameRect appWindow:(AppWindow&)theAppWindow {
    self = [super init];
    if (self) {
        NSWindowStyleMask mask = NSWindowStyleMaskTitled | NSWindowStyleMaskResizable |
                                 NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
        self.window = [[NSWindow alloc] initWithContentRect:frameRect
                                                  styleMask:mask
                                                    backing:NSBackingStoreBuffered
                                                      defer:false];
        self.window.title = @"Simple Text";

        // Bypasses the user's tabbing preference.
        // https://stackoverflow.com/a/40826761/14698275
        self.window.tabbingMode = NSWindowTabbingModeDisallowed;

        openGLView = [[OpenGLView alloc] initWithFrame:frameRect appWindow:theAppWindow];
        self.window.contentView = openGLView;

        [self.window makeFirstResponder:openGLView];
    }
    return self;
}

- (void)showWindow {
    [self.window center];
    // [self.window setFrameAutosaveName:NSBundle.mainBundle.bundleIdentifier];
    [self.window makeKeyAndOrderFront:nil];
}

@end
