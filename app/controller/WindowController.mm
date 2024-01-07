#import "WindowController.h"
#import "model/Rasterizer.h"
#import "util/LogUtil.h"
#import "view/OpenGLLayer.h"

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

        mainView = [[NSView alloc] initWithFrame:frameRect];
        mainView.layer = [OpenGLLayer layer];
        mainView.layer.needsDisplayOnBoundsChange = true;
        // openGLLayer.asynchronous = true;

        self.window.contentView = mainView;

        Rasterizer rasterizer = Rasterizer();
        CGGlyph glyph = rasterizer.get_glyph(@"E");

        RasterizedGlyph rasterizedGlyph = rasterizer.rasterize_glyph(glyph);
        std::vector<uint8_t> buffer = rasterizedGlyph.buffer;
        // logDefault(@"WindowController", @"%dx%d, buffer size = %d", rasterizedGlyph.width,
        //            rasterizedGlyph.height, buffer.size());
        // for (int i = 0; i < buffer.size(); i += 3) {
        //     uint8_t r = buffer[i];
        //     uint8_t g = buffer[i + 1];
        //     uint8_t b = buffer[i + 2];
        //     logDefault(@"Rasterizer", @"%d %d %d", r, g, b);
        // }
    }
    return self;
}

- (void)showWindow {
    [self.window center];
    [self.window setFrameAutosaveName:@"glyph-atlas-cpp"];
    [self.window makeKeyAndOrderFront:nil];
}

@end
