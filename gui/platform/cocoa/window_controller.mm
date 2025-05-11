#include "window_controller.h"

#include "gui/platform/cocoa/gl_view.h"

// Debug use; remove this.
#include "gui/platform/cocoa/debug/view.h"
#include <fmt/base.h>

@interface WindowController () {
    // GLView* opengl_view;
    View* opengl_view;
}

@end

@implementation WindowController

@synthesize appWindow;

- (instancetype)initWithFrame:(NSRect)frameRect
                    appWindow:(gui::WindowWidget*)theAppWindow
                    displayGL:(gui::DisplayGL*)displayGL {
    self = [super init];
    if (self) {
        appWindow = theAppWindow;

        NSWindowStyleMask mask = NSWindowStyleMaskTitled | NSWindowStyleMaskResizable |
                                 NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable;
        self.window = [[[NSWindow alloc] initWithContentRect:frameRect
                                                   styleMask:mask
                                                     backing:NSBackingStoreBuffered
                                                       defer:false] autorelease];
        // opengl_view = [[[GLView alloc] initWithFrame:frameRect
        //                                    appWindow:theAppWindow
        //                                    displayGL:displayGL] autorelease];
        opengl_view = [[[View alloc] initWithFrame:frameRect] autorelease];
        self.window.contentView = opengl_view;
        [self.window makeFirstResponder:opengl_view];

        self.window.delegate = self;
    }
    return self;
}

- (void)windowWillClose:(NSNotification*)notification {
    if (appWindow) {
        appWindow->on_close();
    }
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

- (void)invalidateAppWindowPointer {
    appWindow = nullptr;
    [opengl_view invalidateAppWindowPointer];
}

- (void)setAutoRedraw:(bool)autoRedraw {
    [opengl_view setAutoRedraw:autoRedraw];
}

- (int)framesPerSecond {
    return [opengl_view framesPerSecond];
}

@end
