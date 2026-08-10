#pragma once

#include "base/apple/scoped_cftyperef.h"
#include "base/apple/scoped_cgtyperef.h"
#include "base/strings/sys_string_conversions.h"
#include <AppKit/AppKit.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#include <ImageIO/ImageIO.h>
#include <cstdint>
#include <span>
#include <spdlog/spdlog.h>
#include <string_view>

using base::apple::ScopedCFTypeRef;
using base::apple::ScopedCGColorSpace;
using base::apple::ScopedCGContext;
using base::apple::ScopedCGImage;

void write_png(std::string_view path, CGImageRef image) {
    auto cf_path = base::sys_utf8_to_cfstring_ref(path);
    auto url = ScopedCFTypeRef<CFURLRef>(CFURLCreateWithFileSystemPath(
        kCFAllocatorDefault, cf_path.get(), kCFURLPOSIXPathStyle, false));

    auto dest = ScopedCFTypeRef<CGImageDestinationRef>(
        CGImageDestinationCreateWithURL(url.get(), CFSTR("public.png"), 1, nullptr));
    CGImageDestinationAddImage(dest.get(), image, nullptr);
    CGImageDestinationFinalize(dest.get());
}

struct BitmapView {
    std::span<const uint8_t> pixels;
    size_t width;
    size_t height;
};

void blit_pixels(CGContextRef ctx, const BitmapView& img, double x, double y) {
    const size_t bytes_per_row = img.pixels.size() / img.height;
    auto provider = ScopedCFTypeRef<CGDataProviderRef>(
        CGDataProviderCreateWithData(nullptr, img.pixels.data(), img.pixels.size(), nullptr));
    auto cs = ScopedCGColorSpace(CGColorSpaceCreateDeviceRGB());
    auto cgimg = ScopedCFTypeRef<CGImageRef>(
        CGImageCreate(img.width, img.height, 8, 32, bytes_per_row, cs.get(),
                      kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host, provider.get(),
                      nullptr, false, kCGRenderingIntentDefault));

    const double surface_height = CGBitmapContextGetHeight(ctx);
    const CGRect dst = CGRectMake(x, surface_height - (y + img.height), img.width, img.height);

    CGContextSaveGState(ctx);
    CGContextSetInterpolationQuality(ctx, kCGInterpolationNone);
    CGContextDrawImage(ctx, dst, cgimg.get());
    CGContextRestoreGState(ctx);
}

void show_window(CGContextRef ctx, double scale) {
    auto image = ScopedCGImage(CGBitmapContextCreateImage(ctx));
    // The context is drawn at 2x, so display at half the pixel size to map 1:1 on a Retina screen.
    NSSize size =
        NSMakeSize(CGImageGetWidth(image.get()) / scale, CGImageGetHeight(image.get()) / scale);

    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    NSWindow* window =
        [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 1000, 1000)
                                    styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                                              NSWindowStyleMaskResizable
                                      backing:NSBackingStoreBuffered
                                        defer:NO];

    NSImageView* view =
        [NSImageView imageViewWithImage:[[NSImage alloc] initWithCGImage:image.get() size:size]];
    view.imageAlignment = NSImageAlignTopLeft;
    view.imageScaling = NSImageScaleNone;
    window.contentView = view;
    [window center];
    [window makeKeyAndOrderFront:nil];

    NSMenu* main_menu = [[NSMenu alloc] initWithTitle:@""];
    NSMenuItem* app_item = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    NSMenu* app_menu = [[NSMenu alloc] initWithTitle:@""];
    [app_menu addItem:[[NSMenuItem alloc] initWithTitle:@"Quit"
                                                 action:@selector(terminate:)
                                          keyEquivalent:@"q"]];
    app_item.submenu = app_menu;
    [main_menu addItem:app_item];
    NSApp.mainMenu = main_menu;

    [NSApp activateIgnoringOtherApps:YES];
    [NSApp run];
}

ScopedCGContext create_context(size_t width, size_t height) {
    auto cs = ScopedCGColorSpace(CGColorSpaceCreateDeviceRGB());
    auto ctx = ScopedCGContext(CGBitmapContextCreate(
        /*data=*/nullptr, width, height, 8, /*bytesPerRow=*/0, cs.get(),
        kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host));
    CGContextSetRGBFillColor(ctx.get(), 1, 1, 1, 1);
    CGContextFillRect(ctx.get(), CGRectMake(0, 0, width, height));
    return ctx;
}
