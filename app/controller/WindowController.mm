#import "WindowController.h"
#import "model/Rasterizer.h"
#import "util/LogUtil.h"

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
        self.window.titlebarAppearsTransparent = true;
        self.window.backgroundColor = [NSColor colorWithSRGBRed:228 / 255.f
                                                          green:228 / 255.f
                                                           blue:228 / 255.f
                                                          alpha:1.f];

        mainView = [[View alloc] initWithFrame:frameRect];
        openGLLayer = [OpenGLLayer layer];
        openGLLayer.needsDisplayOnBoundsChange = true;
        // openGLLayer.asynchronous = true;
        mainView.layer = openGLLayer;

        // Fixes blurriness on HiDPI displays.
        // https://bugzilla.gnome.org/show_bug.cgi?id=765194
        mainView.layer.contentsScale = NSScreen.mainScreen.backingScaleFactor;

        self.window.contentView = mainView;
    }
    return self;
}

- (void)showWindow {
    [self.window center];
    [self.window setFrameAutosaveName:self.window.title];
    [self.window makeKeyAndOrderFront:nil];
}

- (void)scrollWheel:(NSEvent*)event {
    if (event.type == NSEventTypeScrollWheel) {
        // openGLLayer->x += event.scrollingDeltaX * NSScreen.mainScreen.backingScaleFactor;
        // if (openGLLayer->x > 0) openGLLayer->x = 0;

        openGLLayer->y += event.scrollingDeltaY * NSScreen.mainScreen.backingScaleFactor;
        if (openGLLayer->y > 0) openGLLayer->y = 0;

        [mainView.layer setNeedsDisplay];
    }
}

- (void)keyDown:(NSEvent*)event {
    NSString* characters = event.characters;
    for (uint32_t k = 0; k < characters.length; k++) {
        char ch = [characters characterAtIndex:k];
        LogDefault(@"WindowController", @"insert char: %c", ch);
        [openGLLayer insertCharacter:ch];
        // unichar key = [characters characterAtIndex:k];
        // switch (key) {
        // case 'k':
        //     openGLLayer->x = 0.0f;
        //     openGLLayer->y = 0.0f;
        //     [mainView.layer setNeedsDisplay];
        //     break;
        // }
        // openGLLayer->x = 0.0f;
        // openGLLayer->y = 0.0f;
        [mainView.layer setNeedsDisplay];
    }
}

- (void)mouseDown:(NSEvent*)event {
    openGLLayer->cursorPoint = NSEvent.mouseLocation;
    [mainView.layer setNeedsDisplay];
}

- (void)rightMouseUp:(NSEvent*)event {
    LogDefault(@"WindowController", @"right click");

    NSMenu* contextMenu = [[NSMenu alloc] initWithTitle:@"Contextual Menu"];
    [contextMenu addItemWithTitle:@"Insert test string"
                           action:@selector(insertTestString)
                    keyEquivalent:@""];
    [contextMenu popUpMenuPositioningItem:nil atLocation:NSEvent.mouseLocation inView:mainView];
}

- (void)insertTestString {
    [openGLLayer insertCharacter:'h'];
    [openGLLayer insertCharacter:'i'];
    [mainView.layer setNeedsDisplay];
}

@end

@implementation View

- (void)viewWillMoveToWindow:(NSWindow*)newWindow {
    NSTrackingArea* trackingArea = [[NSTrackingArea alloc]
        initWithRect:self.bounds
             options:NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved |
                     NSTrackingActiveAlways | NSTrackingInVisibleRect
               owner:self
            userInfo:nil];
    [self addTrackingArea:trackingArea];
}

- (void)mouseEntered:(NSEvent*)event {
    LogDefault(@"View", @"mouse entered");
    [NSCursor.IBeamCursor set];
}

- (void)mouseExited:(NSEvent*)event {
    LogDefault(@"View", @"mouse exited");
    [NSCursor.arrowCursor set];
}

- (void)mouseMoved:(NSEvent*)event {
    LogDefault(@"View", @"mouse moved");
    [NSCursor.IBeamCursor set];
}

@end
