#include "base/apple/scoped_cftyperef.h"
#include "base/apple/scoped_cgtyperef.h"
#include "font/font_rasterizer.h"

#import <Cocoa/Cocoa.h>
#import <CoreText/CoreText.h>

// TODO: Debug use; remove this.
#include <iostream>

using base::apple::ScopedCFTypeRef;
using base::apple::ScopedTypeRef;

namespace font {

class FontRasterizer::impl {
public:
    ScopedCFTypeRef<CTFontRef> ct_font;
    std::vector<ScopedCFTypeRef<CTFontRef>> font_map;

    ScopedCFTypeRef<CTLineRef> createCTLine(std::string_view str8);
};

FontRasterizer::FontRasterizer(const std::string& font_name_utf8, int font_size)
    : pimpl{new impl{}} {
    ScopedCFTypeRef<CFStringRef> ct_font_name{
        CFStringCreateWithCString(nullptr, font_name_utf8.c_str(), kCFStringEncodingUTF8)};
    pimpl->ct_font.reset(CTFontCreateWithName(ct_font_name.get(), font_size, nullptr));

    int ascent = std::ceil(CTFontGetAscent(pimpl->ct_font.get()));
    int descent = std::ceil(CTFontGetDescent(pimpl->ct_font.get()));
    int leading = std::ceil(CTFontGetLeading(pimpl->ct_font.get()));
    int line_height = ascent + descent + leading;

    // TODO: Remove magic numbers that emulate Sublime Text.
    line_height += 1;

    this->line_height = line_height;
    this->descent = -descent;
}

FontRasterizer::~FontRasterizer() {}

FontRasterizer::RasterizedGlyph FontRasterizer::rasterizeUTF8(size_t font_id,
                                                              uint32_t glyph_id) const {
    CTFontRef font_ref = pimpl->font_map[font_id].get();
    CGGlyph glyph_index = glyph_id;

    if (!font_ref) {
        std::cerr << "FontRasterizer::rasterizeUTF8() error: CTFontRef is null!\n";
        std::abort();
    }

    CGRect bounds;
    CTFontGetBoundingRectsForGlyphs(font_ref, kCTFontOrientationDefault, &glyph_index, &bounds, 1);

    int32_t rasterized_left = std::floor(bounds.origin.x);
    uint32_t rasterized_width = std::ceil(bounds.origin.x - rasterized_left + bounds.size.width);
    int32_t rasterized_descent = std::ceil(-bounds.origin.y);
    int32_t rasterized_ascent = std::ceil(bounds.size.height + bounds.origin.y);
    uint32_t rasterized_height = rasterized_descent + rasterized_ascent;

    int32_t top = std::ceil(bounds.size.height + bounds.origin.y);
    top -= descent;

    // If the font is a color font and the glyph doesn't have an outline, it is
    // a color glyph.
    // https://github.com/sublimehq/sublime_text/issues/3747#issuecomment-726837744
    bool colored_font = CTFontGetSymbolicTraits(font_ref) & kCTFontTraitColorGlyphs;
    bool has_outline = CTFontCreatePathForGlyph(font_ref, glyph_index, nullptr);
    bool colored = colored_font && !has_outline;

    ScopedTypeRef<CGColorSpaceRef> color_space_ref{CGColorSpaceCreateDeviceRGB()};
    ScopedTypeRef<CGContextRef> context{CGBitmapContextCreate(
        nullptr, rasterized_width, rasterized_height, 8, rasterized_width * 4,
        color_space_ref.get(), kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host)};

    CGFloat alpha = colored ? 0.0 : 1.0;
    CGContextSetRGBFillColor(context.get(), 0.0, 0.0, 0.0, alpha);

    CGContextFillRect(context.get(), CGRectMake(0.0, 0.0, rasterized_width, rasterized_height));
    CGContextSetAllowsFontSmoothing(context.get(), true);
    CGContextSetShouldSmoothFonts(context.get(), false);
    CGContextSetAllowsFontSubpixelQuantization(context.get(), true);
    CGContextSetShouldSubpixelQuantizeFonts(context.get(), true);
    CGContextSetAllowsFontSubpixelPositioning(context.get(), true);
    CGContextSetShouldSubpixelPositionFonts(context.get(), true);
    CGContextSetAllowsAntialiasing(context.get(), true);
    CGContextSetShouldAntialias(context.get(), true);

    CGContextSetRGBFillColor(context.get(), 1.0, 1.0, 1.0, 1.0);
    CGPoint rasterization_origin = CGPointMake(-rasterized_left, rasterized_descent);

    CTFontDrawGlyphs(font_ref, &glyph_index, &rasterization_origin, 1, context.get());

    uint8_t* bitmap_data = (uint8_t*)CGBitmapContextGetData(context.get());
    size_t height = CGBitmapContextGetHeight(context.get());
    size_t bytes_per_row = CGBitmapContextGetBytesPerRow(context.get());
    size_t len = height * bytes_per_row;

    size_t pixels = len / 4;
    std::vector<uint8_t> buffer;
    size_t size = colored ? pixels * 4 : pixels * 3;

    // TODO: This assumes little endian; detect and support big endian.
    buffer.reserve(size);
    for (size_t i = 0; i < pixels; i++) {
        size_t offset = i * 4;
        buffer.emplace_back(bitmap_data[offset + 2]);
        buffer.emplace_back(bitmap_data[offset + 1]);
        buffer.emplace_back(bitmap_data[offset]);
        if (colored) {
            buffer.emplace_back(bitmap_data[offset + 3]);
        }
    }

    double advance =
        CTFontGetAdvancesForGlyphs(font_ref, kCTFontOrientationDefault, &glyph_index, nullptr, 1);

    return {
        .colored = colored,
        .left = rasterized_left,
        .top = top,
        .width = static_cast<int32_t>(rasterized_width),
        .height = static_cast<int32_t>(rasterized_height),
        .advance = static_cast<int32_t>(std::ceil(advance)),
        .buffer = buffer,
    };
}

FontRasterizer::LineLayout FontRasterizer::layoutLine(std::string_view str8) const {
    std::vector<ShapedRun> runs;

    ScopedCFTypeRef<CTLineRef> ct_line = pimpl->createCTLine(str8);

    CFArrayRef run_array = CTLineGetGlyphRuns(ct_line.get());
    CFIndex run_count = CFArrayGetCount(run_array);
    for (CFIndex i = 0; i < run_count; i++) {
        CTRunRef ct_run = (CTRunRef)CFArrayGetValueAtIndex(run_array, i);

        // Cache font in font map.
        ScopedCFTypeRef<CTFontRef> ct_font =
            (CTFontRef)CFDictionaryGetValue(CTRunGetAttributes(ct_run), kCTFontAttributeName);
        size_t font_id = pimpl->font_map.size();
        pimpl->font_map.push_back(std::move(ct_font));

        // TODO: Debug use; remove this.
        CTFontRef temp = pimpl->font_map[font_id].get();
        CFStringRef ct_font_name = CTFontCopyPostScriptName(temp);
        std::cerr << ((NSString*)ct_font_name).UTF8String << '\n';

        CFIndex glyph_count = CTRunGetGlyphCount(ct_run);
        std::vector<CGGlyph> glyph_ids(glyph_count);
        std::vector<CFIndex> indices(glyph_count);
        std::vector<CGPoint> positions(glyph_count);
        std::vector<CGSize> advances(glyph_count);

        CTRunGetGlyphs(ct_run, {0, glyph_count}, glyph_ids.data());
        CTRunGetStringIndices(ct_run, {0, glyph_count}, indices.data());
        CTRunGetPositions(ct_run, {0, glyph_count}, positions.data());
        CTRunGetAdvances(ct_run, {0, glyph_count}, advances.data());

        std::vector<ShapedGlyph> glyphs;
        for (size_t i = 0; i < glyph_count; i++) {
            ShapedGlyph glyph{
                .glyph_id = glyph_ids[i],
                // TODO: Use subpixel variants instead of rounding.
                .position =
                    Point{
                        .x = static_cast<int>(std::ceil(positions[i].x)),
                        .y = static_cast<int>(std::ceil(positions[i].y)),
                    },
                // TODO: Properly calculate UTF-8 index.
                .index = static_cast<size_t>(indices[i]),
            };
            glyphs.push_back(std::move(glyph));
        }

        runs.reserve(glyph_count);
        ShapedRun run{
            .font_id = font_id,
            .glyphs = std::move(glyphs),
        };
        runs.push_back(std::move(run));
    }

    double width = CTLineGetTypographicBounds(ct_line.get(), nullptr, nullptr, nullptr);
    return {
        .width = static_cast<int>(std::ceil(width)),
        .runs = std::move(runs),
    };
}

ScopedCFTypeRef<CTLineRef> FontRasterizer::impl::createCTLine(std::string_view str8) {
    ScopedCFTypeRef<CFStringRef> text_string{
        CFStringCreateWithBytesNoCopy(kCFAllocatorDefault, (const uint8_t*)&str8[0], str8.length(),
                                      kCFStringEncodingUTF8, false, kCFAllocatorNull)};

    ScopedCFTypeRef<CFMutableDictionaryRef> attr{CFDictionaryCreateMutable(
        kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks)};
    CFDictionaryAddValue(attr.get(), kCTFontAttributeName, ct_font.get());

    ScopedCFTypeRef<CFAttributedStringRef> attr_string{
        CFAttributedStringCreate(kCFAllocatorDefault, text_string.get(), attr.get())};

    return CTLineCreateWithAttributedString(attr_string.get());
}

}  // namespace font
