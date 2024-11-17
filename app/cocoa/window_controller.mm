#include "window_controller.h"

#include "app/cocoa/gl_view.h"

@interface WindowController () {
    GLView* opengl_view;
    app::Window* app_window;
}
@end

@implementation WindowController

- (instancetype)initWithFrame:(NSRect)frameRect
                    appWindow:(app::Window*)appWindow
                    displayGl:(app::DisplayGL*)displayGl {
    self = [super init];
    if (self) {
        app_window = appWindow;

        NSWindowStyleMask mask = NSWindowStyleMaskTitled | NSWindowStyleMaskResizable |
                                 NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
        self.window = [[[NSWindow alloc] initWithContentRect:frameRect
                                                   styleMask:mask
                                                     backing:NSBackingStoreBuffered
                                                       defer:false] autorelease];
        opengl_view = [[[GLView alloc] initWithFrame:frameRect
                                           appWindow:appWindow
                                           displaygl:displayGl] autorelease];
        self.window.contentView = opengl_view;
        [self.window makeFirstResponder:opengl_view];

        // Bypass the user's tabbing preference.
        // https://stackoverflow.com/a/40826761/14698275
        self.window.tabbingMode = NSWindowTabbingModeDisallowed;

        self.window.delegate = self;
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

- (void)setTitle:(std::string_view)title {
    self.window.title = [NSString stringWithUTF8String:title.data()];
}

- (void)setFilePath:(std::string_view)path {
    self.window.representedFilename = [NSString stringWithUTF8String:path.data()];
}

- (app::Window*)getAppWindow {
    return app_window;
}

- (NSWindow*)getNsWindow {
    return self.window;
}

@end
