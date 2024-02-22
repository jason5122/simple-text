#import "WindowController.h"

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
        self.window.title = @"Simple Text";
        // self.window.titlebarAppearsTransparent = true;
        // self.window.backgroundColor = [NSColor colorWithSRGBRed:228 / 255.f
        //                                                   green:228 / 255.f
        //                                                    blue:228 / 255.f
        //                                                   alpha:1.f];

        editorView = [[EditorView alloc] initWithFrame:frameRect];
        editorViewNew = [[EditorViewNew alloc] initWithFrame:frameRect];
        self.window.contentView = editorView;
        // self.window.contentView = editorViewNew;

        [self.window makeFirstResponder:editorView];
        // [self.window makeFirstResponder:editorViewNew];
    }
    return self;
}

- (void)showWindow {
    [self.window center];
    [self.window setFrameAutosaveName:self.window.title];
    [self.window makeKeyAndOrderFront:nil];
}

@end
