#include "base/apple/scoped_cftyperef.h"
#include "base/apple/scoped_cgtyperef.h"
#include "experiments/rasterizer/font.h"
#include "experiments/rasterizer/mac_helpers.h"
#include <AppKit/AppKit.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#include <ImageIO/ImageIO.h>
#include <cmath>
#include <cstdint>
#include <spdlog/spdlog.h>
#include <string_view>
#include <vector>

using base::apple::ScopedCFTypeRef;
using base::apple::ScopedCGColorSpace;
using base::apple::ScopedCGContext;
using base::apple::ScopedCGImage;

namespace {

void draw_text(CGContextRef ctx, const font::ShapedLine& shaped, int i) {
    constexpr int scale = 2;

    const double line_height =
        std::ceil(shaped.ascent) + std::ceil(shaped.descent) + std::ceil(shaped.leading);

    font::GlyphRasterizer rasterizer;
    for (const auto& run : shaped.runs) {
        for (const auto& g : run.glyphs) {
            double pos_x = g.x_offset;
            double pos_y = g.y_offset + (1000.0 / scale) - std::ceil(shaped.ascent);
            pos_y -= line_height * i;

            font::GlyphBitmap bmp = rasterizer.rasterize(run.font, g.glyph, scale);

            ImageView img = {
                .data = bmp.pixels.data(),
                .width = bmp.width,
                .height = bmp.height,
                .stride = bmp.width * bmp.bytes_per_pixel,
            };

            // Debug use.
            constexpr long kMarginLeftPx = 2;
            constexpr long kMarginTopPx = 124;

            // Snap the pen to a whole device pixel (ST renders at phase 0 and blits at an integer
            // pen); bearing_x/y are already integer device px. Sub-pixel glyph placement (which ST
            // does at ~half-pixel phases, e.g. 8/13pt) isn't reproducible with CGContextDrawImage
            // and belongs to the future OpenGL atlas path, not here.
            long px = std::lround(pos_x * scale) + bmp.bearing_x + kMarginLeftPx;
            long py = std::lround(pos_y * scale) + bmp.bearing_y - kMarginTopPx;
            CGRect rect = CGRectMake((double)px / scale, (double)py / scale,
                                     (double)bmp.width / scale, (double)bmp.height / scale);
            draw_pixels(ctx, img, rect);
        }
    }
}

void show_window(CGContextRef ctx) {
    auto image = ScopedCGImage(CGBitmapContextCreateImage(ctx));
    // The context is drawn at 2x, so display at half the pixel size to map 1:1 on a Retina screen.
    NSSize size =
        NSMakeSize(CGImageGetWidth(image.get()) / 2.0, CGImageGetHeight(image.get()) / 2.0);

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

}  // namespace

int main() {
    auto text = "Sphinx of black quartz, judge my vow 😀. 0123456789";
    auto family = "Source Code Pro";
    double font_size = 16;
    auto out_path = "out.png";

    constexpr size_t width = 2000;
    constexpr size_t height = 1000;
    constexpr size_t scale = 2;

    constexpr size_t bytes_per_pixel = 4;
    constexpr size_t bytes_per_row = width * bytes_per_pixel;
    std::vector<uint8_t> pixels(height * bytes_per_row);

    auto cs = ScopedCGColorSpace(CGColorSpaceCreateDeviceRGB());
    auto ctx = ScopedCGContext(
        CGBitmapContextCreate(pixels.data(), width, height, 8, bytes_per_row, cs.get(),
                              kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host));

    CGContextSetRGBFillColor(ctx.get(), 1, 1, 1, 1);
    CGContextFillRect(ctx.get(), CGRectMake(0, 0, width, height));
    CGContextScaleCTM(ctx.get(), scale, scale);

    font::FontDatabase db;
    auto face = db.match({family, font::Weight::Normal, font::Slant::Normal});
    if (!face) return 1;
    auto handle = db.create_font(*face, font_size);
    if (!handle) return 1;

    font::TextShaper shaper;

    std::vector<std::string> lines = {
        "Sphinx of black quartz, judge my vow! 😀😀😀",
        "The quick brown fox jumps over the lazy dog. 你好",
        "",
        "Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod",
        "tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam,",
        "quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo",
        "consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse",
        "cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non",
        "proident, sunt in culpa qui officia deserunt mollit anim id est laborum.",
    };
    for (int i = 0; i < lines.size(); i++) {
        auto shaped = shaper.shape(*handle, lines[i]);
        auto& [runs, line_width, ascent, descent, leading] = shaped;
        draw_text(ctx.get(), shaped, i);
    }

    show_window(ctx.get());

    // Debug.
    // auto img = ScopedCGImage(CGBitmapContextCreateImage(ctx.get()));
    // write_png(out_path, img.get());
}
