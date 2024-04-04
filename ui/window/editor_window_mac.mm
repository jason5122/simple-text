#include "ui/window/editor_window.h"
#include <AppKit/AppKit.h>
#include <Cocoa/Cocoa.h>

@interface WindowController2 : NSWindowController

- (instancetype)initWithFrame:(NSRect)frameRect;

- (void)showWindow;

@end

@implementation WindowController2

- (instancetype)initWithFrame:(NSRect)frameRect {
    self = [super init];
    if (self) {
        NSWindowStyleMask mask = NSWindowStyleMaskTitled | NSWindowStyleMaskResizable |
                                 NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
        self.window = [[NSWindow alloc] initWithContentRect:frameRect
                                                  styleMask:mask
                                                    backing:NSBackingStoreBuffered
                                                      defer:false];
        self.window.title = @"Simple Text";
        // self.window.titlebarAppearsTransparent = true;
        // self.window.backgroundColor = [NSColor colorWithSRGBRed:228 / 255.f
        //                                                   green:228 / 255.f
        //                                                    blue:228 / 255.f
        //                                                   alpha:1.f];
    }
    return self;
}

- (void)showWindow {
    [self.window center];
    [self.window setFrameAutosaveName:NSBundle.mainBundle.bundleIdentifier];
    [self.window makeKeyAndOrderFront:nil];
}

@end

class EditorWindow2::impl {
public:
    WindowController2* window_controller;
};

EditorWindow2::EditorWindow2() : pimpl{new impl{}} {
    NSRect frameRect = NSMakeRect(0, 0, 600, 500);
    pimpl->window_controller = [[WindowController2 alloc] initWithFrame:frameRect];
}

void EditorWindow2::show() {
    [pimpl->window_controller showWindow];
}
