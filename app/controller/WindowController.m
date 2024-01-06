#import "WindowController.h"
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

        NSDictionary* descriptorOptions = @{(id)kCTFontFamilyNameAttribute : @"Menlo"};
        CTFontDescriptorRef descriptor =
            CTFontDescriptorCreateWithAttributes((CFDictionaryRef)descriptorOptions);
        CFTypeRef keys[] = {kCTFontFamilyNameAttribute};
        CFSetRef mandatoryAttrs = CFSetCreate(kCFAllocatorDefault, keys, 1, &kCFTypeSetCallBacks);
        CFArrayRef fontDescriptors =
            CTFontDescriptorCreateMatchingFontDescriptors(descriptor, NULL);
        for (NSFontDescriptor* descriptor in (NSArray*)CFBridgingRelease(fontDescriptors)) {
            NSString* familyName = [descriptor objectForKey:NSFontFamilyAttribute];
            NSString* fontSize = [descriptor objectForKey:NSFontFaceAttribute];
            logDefault(@"WindowController", @"%@ %@", familyName, fontSize);
        }
    }
    return self;
}

- (void)showWindow {
    [self.window center];
    [self.window setFrameAutosaveName:@"glyph-atlas-cpp"];
    [self.window makeKeyAndOrderFront:nil];
}

@end
