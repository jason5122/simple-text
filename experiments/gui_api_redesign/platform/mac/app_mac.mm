#include "experiments/gui_api_redesign/app.h"

#include <AppKit/AppKit.h>

constexpr bool kBenchmarkMode = true;
constexpr bool kUseOpenGL = true;

@interface View : NSView
- (instancetype)initWithFrame:(NSRect)frame;
@end

@implementation View
- (instancetype)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    return self;
}
- (void)drawRect:(NSRect)dirtyRect {
    if constexpr (kBenchmarkMode) {
        [NSApp terminate:nil];
    }
}
@end

@interface GLLayer : CAOpenGLLayer
@end

@implementation GLLayer
- (BOOL)canDrawInCGLContext:(CGLContextObj)glContext
                pixelFormat:(CGLPixelFormatObj)pixelFormat
               forLayerTime:(CFTimeInterval)timeInterval
                displayTime:(const CVTimeStamp*)timeStamp {
    return true;
}

- (void)drawInCGLContext:(CGLContextObj)glContext
             pixelFormat:(CGLPixelFormatObj)pixelFormat
            forLayerTime:(CFTimeInterval)timeInterval
             displayTime:(const CVTimeStamp*)timeStamp {
    if constexpr (kBenchmarkMode) {
        [NSApp terminate:nil];
    }
}
@end

int App::run() {
    @autoreleasepool {
        [NSApplication sharedApplication];

        // Add Command+Q to quit.
        NSMenu* main_menu = [[NSMenu alloc] initWithTitle:@""];
        NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
        NSMenu* submenu = [[NSMenu alloc] initWithTitle:@""];
        [submenu addItem:[[NSMenuItem alloc] initWithTitle:@"Quit"
                                                    action:@selector(terminate:)
                                             keyEquivalent:@"q"]];
        item.submenu = submenu;
        [main_menu addItem:item];
        NSApp.mainMenu = main_menu;

        NSRect frame = NSMakeRect(0, 0, 1200, 800);
        NSUInteger style =
            NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable;
        NSWindow* window = [[[NSWindow alloc] initWithContentRect:frame
                                                        styleMask:style
                                                          backing:NSBackingStoreBuffered
                                                            defer:false] autorelease];

        // Create view (OpenGL or non-OpenGL).
        if constexpr (kUseOpenGL) {
            NSView* gl_view = [[[NSView alloc] initWithFrame:frame] autorelease];
            gl_view.layer = [[GLLayer alloc] init];
            gl_view.layer.needsDisplayOnBoundsChange = true;
            window.contentView = gl_view;
        } else {
            window.contentView = [[[View alloc] initWithFrame:frame] autorelease];
        }

        [window setTitle:@"GUI API Redesign"];
        [window makeKeyAndOrderFront:nil];

        [NSApp run];
    }
    return 0;  // TODO: How do we get non-zero return values from NSApp?
}

Window& App::create_window(int width, int height) {
    auto win = std::make_unique<Window>(width, height);
    Window& ref = *win;
    windows_.push_back(std::move(win));
    return ref;
}
