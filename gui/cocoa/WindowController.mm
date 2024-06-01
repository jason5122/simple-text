#include "WindowController.h"
#include "gui/cocoa/OpenGLView.h"

@interface WindowController () {
    OpenGLView* opengl_view;
    gui::Window* app_window;
}
@end

@implementation WindowController

- (instancetype)initWithFrame:(NSRect)frameRect
                    appWindow:(gui::Window*)appWindow
                    displayGl:(gui::DisplayGL*)displayGl {
    self = [super init];
    if (self) {
        app_window = appWindow;

        NSWindowStyleMask mask = NSWindowStyleMaskTitled | NSWindowStyleMaskResizable |
                                 NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
        self.window = [[[NSWindow alloc] initWithContentRect:frameRect
                                                   styleMask:mask
                                                     backing:NSBackingStoreBuffered
                                                       defer:false] autorelease];
        opengl_view = [[[OpenGLView alloc] initWithFrame:frameRect
                                               appWindow:appWindow
                                               displaygl:displayGl] autorelease];
        self.window.contentView = opengl_view;
        [self.window makeFirstResponder:opengl_view];

        // Bypass the user's tabbing preference.
        // https://stackoverflow.com/a/40826761/14698275
        self.window.tabbingMode = NSWindowTabbingModeDisallowed;

        self.window.delegate = self;

        self.window.representedFilename = @"/Users/jason/cs/side-projects/simple-text/BUILD.gn";

        self.window.title = @"BUILD.gn — simple-text";
    }
    return self;
}

- (void)windowWillClose:(NSNotification*)notification {
    app_window->onClose();
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

- (int)getWidth {
    return opengl_view.frame.size.width;
}

- (int)getHeight {
    return opengl_view.frame.size.height;
}

- (int)getScaleFactor {
    return opengl_view.layer.contentsScale;
}

- (bool)isDarkMode {
    return opengl_view.effectiveAppearance.name == NSAppearanceNameDarkAqua;
}

@end
