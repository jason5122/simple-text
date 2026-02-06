#include <Cocoa/Cocoa.h>
#include <QuartzCore/QuartzCore.h>

#include <iostream>

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
    std::cout << "draw" << std::endl;
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
    std::cout << "draw" << std::endl;
    if constexpr (kBenchmarkMode) {
        [NSApp terminate:nil];
    }
}
@end

int main(int argc, const char* argv[]) {
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
        NSWindow* window = [[NSWindow alloc] initWithContentRect:frame
                                                       styleMask:style
                                                         backing:NSBackingStoreBuffered
                                                           defer:false];

        // Create view (OpenGL or non-OpenGL).
        if constexpr (kUseOpenGL) {
            NSView* gl_view = [[NSView alloc] initWithFrame:frame];
            gl_view.layer = [[GLLayer alloc] init];
            gl_view.layer.needsDisplayOnBoundsChange = true;
            window.contentView = gl_view;
        } else {
            window.contentView = [[View alloc] initWithFrame:frame];
        }

        [window setTitle:@"Bare macOS App"];
        NSApp.activationPolicy = NSApplicationActivationPolicyRegular;
        [window makeKeyAndOrderFront:nil];

        [NSApp run];
    }
    return 0;
}
