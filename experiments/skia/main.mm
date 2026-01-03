#include "base/debug/profiler.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkRect.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/utils/mac/SkCGUtils.h"
#include <Cocoa/Cocoa.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreVideo/CoreVideo.h>
#include <QuartzCore/QuartzCore.h>
#include <mach/mach_time.h>

// If you want your earlier Op() sample, keep these includes + link pathops.
// #include "third_party/skia/include/core/SkPath.h"
// #include "third_party/skia/include/core/SkPathBuilder.h"
// #include "third_party/skia/include/pathops/SkPathOps.h"

static double SecondsSince(uint64_t t0) {
    static mach_timebase_info_data_t tb = {0, 0};
    if (tb.denom == 0) mach_timebase_info(&tb);
    uint64_t now = mach_absolute_time();
    double ns = (double)(now - t0) * (double)tb.numer / (double)tb.denom;
    return ns * 1e-9;
}

// Copy pixels into an owned SkBitmap, then create CGImage from that.
// This avoids the “blank/intermittent” bug from handing CA borrowed pixels.
static CGImageRef MakeCGImageFromSurfaceCopy(const sk_sp<SkSurface>& surface) {
    const int w = surface->width();
    const int h = surface->height();

    SkImageInfo info = SkImageInfo::MakeN32Premul(w, h);

    SkBitmap bm;
    if (!bm.tryAllocPixels(info)) return nullptr;

    if (!surface->readPixels(bm, 0, 0)) return nullptr;

    return SkCreateCGImageRef(bm);  // you own the returned CGImageRef
}

@interface SkiaLayer : CALayer {
@private
    CVDisplayLinkRef _dl;
    uint64_t _t0;
}
@end

static CVReturn DisplayLinkCB(CVDisplayLinkRef,
                              const CVTimeStamp*,
                              const CVTimeStamp*,
                              CVOptionFlags,
                              CVOptionFlags*,
                              void* ctx) {
    SkiaLayer* layer = (__bridge SkiaLayer*)ctx;
    dispatch_async(dispatch_get_main_queue(), ^{
      [layer setNeedsDisplay];
    });
    return kCVReturnSuccess;
}

@implementation SkiaLayer

// Avoid implicit animations while resizing.
// TODO: This looks very useful! Can we learn from this and apply it elsewhere?
// https://stackoverflow.com/a/16668169
// https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/CoreAnimation_guide/ReactingtoLayerChanges/ReactingtoLayerChanges.html
- (id<CAAction>)actionForKey:(NSString*)event {
    // return nil;
    return (id)[NSNull null];
}

- (instancetype)init {
    self = [super init];
    if (self) {
        self.contentsScale = NSScreen.mainScreen.backingScaleFactor;

        // Redraw on resize.
        self.needsDisplayOnBoundsChange = YES;
        self.autoresizingMask = kCALayerWidthSizable | kCALayerHeightSizable;

        _t0 = mach_absolute_time();

        // Redraw on vsync.
        CVDisplayLinkCreateWithActiveCGDisplays(&_dl);
        CVDisplayLinkSetOutputCallback(_dl, &DisplayLinkCB, (__bridge void*)self);
        CVDisplayLinkStart(_dl);
    }
    return self;
}

- (void)dealloc {
    if (_dl) {
        CVDisplayLinkStop(_dl);
        CVDisplayLinkRelease(_dl);
        _dl = NULL;
    }
}

- (void)display {
    auto prof = base::Profiler("display()");

    CGSize sizePt = self.bounds.size;
    CGFloat scale = self.contentsScale > 0 ? self.contentsScale : 1.0;
    int w = (int)lrint(sizePt.width * scale);
    int h = (int)lrint(sizePt.height * scale);
    if (w <= 0 || h <= 0) return;

    // Time and looping phase
    double t = SecondsSince(_t0);
    double u = fmod(t, 2.0) / 2.0;  // 0..1 over 2 seconds
    double a = u * 2.0 * M_PI;

    float dx = (float)(sin(a) * (w * 0.20));  // move left-right
    float rotDeg = (float)(a * 180.0 / M_PI);

    // Skia CPU raster surface (new API)
    SkImageInfo info = SkImageInfo::MakeN32Premul(w, h);
    auto surface = SkSurfaces::Raster(info);
    if (!surface) return;

    SkCanvas* c = surface->getCanvas();
    c->clear(SK_ColorWHITE);

    SkPaint p;
    p.setAntiAlias(true);

    // Draw animated geometry
    c->save();
    // c->translate(w - 400, h - 400);  // Test left/top resizing.
    c->translate(400, 400);  // Test right/bottom resizing.
    c->rotate(rotDeg);

    p.setStyle(SkPaint::kFill_Style);
    p.setColor(0xFF000000);
    c->drawRect(SkRect::MakeXYWH(-90, -50, 180, 100), p);

    p.setStyle(SkPaint::kStroke_Style);
    p.setStrokeWidth(8.0f * scale);
    p.setColor(0xFF808080);
    c->drawCircle(0, 0, 200, p);

    c->restore();

    // Bridge to CALayer contents
    CGImageRef cg = MakeCGImageFromSurfaceCopy(surface);
    if (!cg) return;

    // Avoid implicit animations while resizing.
    // TODO: This CATransaction trick looks useful. Can we learn from this and apply it elsewhere?
    // [CATransaction begin];
    // [CATransaction setDisableActions:YES];
    self.contents = (__bridge id)cg;
    // [CATransaction commit];

    // TODO: Check out ScopedCAActionDisabler!
    // https://source.chromium.org/chromium/chromium/src/+/main:ui/base/cocoa/animation_utils.h
    // TODO: There seems to be evidence that this is how Chromium solves resize jitter!
    // https://source.chromium.org/chromium/chromium/src/+/main:ui/accelerated_widget_mac/ca_layer_tree_coordinator.mm;l=175?q=ca_layer_tree_coordinator

    CGImageRelease(cg);
}

@end

@interface AppDelegate : NSObject <NSApplicationDelegate>
@property(strong) NSWindow* window;
@end

@implementation AppDelegate
@synthesize window = _window;

- (void)applicationDidFinishLaunching:(NSNotification*)note {
    NSRect frame = NSMakeRect(200, 200, 720, 480);
    self.window = [[NSWindow alloc]
        initWithContentRect:frame
                  styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                             NSWindowStyleMaskResizable)
                    backing:NSBackingStoreBuffered
                      defer:NO];
    self.window.title = @"Skia + CALayer (CPU) animation";

    NSView* content = self.window.contentView;
    content.wantsLayer = YES;

    SkiaLayer* layer = [SkiaLayer layer];
    layer.frame = content.bounds;

    [content.layer addSublayer:layer];

    [self.window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
    return YES;
}

@end

int main(int argc, const char* argv[]) {
    // Disable stdout buffering.
    std::setbuf(stdout, nullptr);

    @autoreleasepool {
        NSApplication* app = NSApplication.sharedApplication;
        AppDelegate* delegate = [AppDelegate new];
        app.delegate = delegate;

        NSMenu* main_menu = [[NSMenu alloc] initWithTitle:@""];
        NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
        NSMenu* submenu = [[NSMenu alloc] initWithTitle:@""];
        [submenu addItem:[[NSMenuItem alloc] initWithTitle:@"Quit"
                                                    action:@selector(terminate:)
                                             keyEquivalent:@"q"]];
        item.submenu = submenu;
        [main_menu addItem:item];
        NSApp.mainMenu = main_menu;

        NSApp.activationPolicy = NSApplicationActivationPolicyRegular;

        [app run];
    }
    return 0;
}
