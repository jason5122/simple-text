#include "base/apple/scoped_cftyperef.h"
#include "base/apple/scoped_cgtyperef.h"
#include "font/font_rasterizer.h"
#include "font/utf16_to_utf8_indices_map.h"
#include "unicode/SkTFitsIn.h"
#include "unicode/unicode.h"

#import <Cocoa/Cocoa.h>
#import <CoreText/CoreText.h>

// TODO: Debug use; remove this.
#include <cassert>
#include <format>
#include <iostream>
#include <ranges>

using base::apple::ScopedCFTypeRef;
using base::apple::ScopedTypeRef;

namespace font {

class FontRasterizer::impl {
public:
    std::unordered_map<std::string, size_t> font_postscript_name_to_id;
    std::vector<ScopedCFTypeRef<CTFontRef>> font_id_to_native;
    std::vector<Metrics> font_id_to_metrics;
    size_t cacheFont(CTFontRef ct_font);

    ScopedCFTypeRef<CTLineRef> createCTLine(size_t font_id, std::string_view str8);
};

FontRasterizer::FontRasterizer() : pimpl{new impl{}} {}

FontRasterizer::~FontRasterizer() {}

size_t FontRasterizer::addFont(const std::string& font_name_utf8, int font_size) {
    ScopedCFTypeRef<CFStringRef> ct_font_name{
        CFStringCreateWithCString(nullptr, font_name_utf8.c_str(), kCFStringEncodingUTF8)};
    CTFontRef ct_font = CTFontCreateWithName(ct_font_name.get(), font_size, nullptr);
    return pimpl->cacheFont(ct_font);
}

const FontRasterizer::Metrics& FontRasterizer::getMetrics(size_t font_id) const {
    return pimpl->font_id_to_metrics.at(font_id);
}

RasterizedGlyph FontRasterizer::rasterizeUTF8(size_t font_id, uint32_t glyph_id) const {
    CTFontRef font_ref = pimpl->font_id_to_native[font_id].get();

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
    for (size_t i = 0; i < pixels; ++i) {
        size_t offset = i * 4;
        buffer.emplace_back(bitmap_data[offset + 2]);
        buffer.emplace_back(bitmap_data[offset + 1]);
        buffer.emplace_back(bitmap_data[offset]);
        if (colored) {
            buffer.emplace_back(bitmap_data[offset + 3]);
        }
    }

    return {
        .colored = colored,
        .left = rasterized_left,
        .top = top,
        .width = static_cast<int32_t>(rasterized_width),
        .height = static_cast<int32_t>(rasterized_height),
        .buffer = buffer,
    };
}

// https://skia.googlesource.com/skia/+/0a7c7b0b96fc897040e71ea3304d9d6a042cda8b/modules/skshaper/src/SkShaper_coretext.cpp#195
LineLayout FontRasterizer::layoutLine(size_t font_id, std::string_view str8) const {
    assert(std::ranges::count(str8, '\n') == 0);

    UTF16ToUTF8IndicesMap utf8IndicesMap;
    if (!utf8IndicesMap.setUTF8(str8.data(), str8.length())) {
        std::cerr << "UTF16ToUTF8IndicesMap::setUTF8 error\n";
        std::abort();
    }

    ScopedCFTypeRef<CTLineRef> ct_line = pimpl->createCTLine(font_id, str8);

    int total_advance = 0;
    std::vector<ShapedRun> runs;
    CFArrayRef run_array = CTLineGetGlyphRuns(ct_line.get());
    CFIndex run_count = CFArrayGetCount(run_array);

    ShapedGlyph* prev_glyph = nullptr;

    for (CFIndex i = 0; i < run_count; ++i) {
        CTRunRef ct_run = (CTRunRef)CFArrayGetValueAtIndex(run_array, i);

        CTFontRef ct_font =
            (CTFontRef)CFDictionaryGetValue(CTRunGetAttributes(ct_run), kCTFontAttributeName);
        size_t run_font_id = pimpl->cacheFont(ct_font);

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
        glyphs.reserve(glyph_count);
        for (CFIndex i = 0; i < glyph_count; ++i) {
            // TODO: Use subpixel variants instead of rounding.
            Point position = {
                .x = total_advance,
                // .x = static_cast<int>(std::ceil(positions[i].x)),
                .y = static_cast<int>(std::ceil(positions[i].y)),
            };
            Point advance = {
                .x = static_cast<int>(std::ceil(advances[i].width)),
                .y = static_cast<int>(std::ceil(advances[i].height)),
            };

            size_t utf8_index = utf8IndicesMap.mapIndex(indices[i]);
            ShapedGlyph glyph{
                .glyph_id = glyph_ids[i],
                .position = position,
                .advance = advance,
                .index = utf8_index,
            };
            glyphs.push_back(std::move(glyph));

            // Set previous glyph's length. We require the next index, so we have to do this late.
            if (prev_glyph) {
                prev_glyph->length = utf8_index - prev_glyph->index;
            }
            prev_glyph = &glyphs[i];

            total_advance += advance.x;
        }

        runs.emplace_back(ShapedRun{run_font_id, std::move(glyphs)});
    }
    // Set previous glyph's length. We require the next index, so we have to do this late.
    if (prev_glyph) {
        prev_glyph->length = str8.length() - prev_glyph->index;
    }

    // Fetch ascent from the main line layout font. Otherwise, the baseline will shift up and down
    // when fonts with different ascents mix (e.g., emoji being taller than plain text).
    int ascent = getMetrics(font_id).ascent;

    // TODO: Currently, width != sum of all advances since we round. When we implement subpixel
    // variants, this should no longer be an issue.
    // double width = CTLineGetTypographicBounds(ct_line.get(), nullptr, nullptr, nullptr);

    // TODO: See if this is correct.
    // if (total_advance % 2 == 1) {
    //     total_advance += 1;
    // }

    return {
        .layout_font_id = font_id,
        .width = total_advance,
        // .width = static_cast<int>(std::ceil(width)),
        .length = str8.length(),
        .runs = std::move(runs),
        .ascent = static_cast<int>(ascent),
    };
}

size_t FontRasterizer::impl::cacheFont(CTFontRef ct_font) {
    CFStringRef ct_font_name = CTFontCopyPostScriptName(ct_font);
    NSString* ns_font_name = (NSString*)ct_font_name;
    std::string font_name(ns_font_name.UTF8String);

    // Sometimes, the CFStringRef isn't convertible to std::string. We need to bridge it to an
    // NSString first.
    if (font_name.empty()) {
        std::cerr << "FontRasterizer::impl::cacheFont() error: font_name is empty\n";
        std::abort();
    }

    if (!font_postscript_name_to_id.contains(font_name)) {
        // TODO: Figure out how to automatically retain using ScopedCFTypeRef.
        ct_font = (CTFontRef)CFRetain(ct_font);

        int ascent = std::ceil(CTFontGetAscent(ct_font));
        int descent = std::ceil(CTFontGetDescent(ct_font));
        int leading = std::ceil(CTFontGetLeading(ct_font));

        if (ascent % 2 == 1) {
            ascent += 1;
        }
        if (descent % 2 == 1) {
            descent += 1;
        }

        int line_height = ascent + descent + leading;

        Metrics metrics{
            .font_size = 0,  // TODO: Calculate font size correctly.
            .line_height = line_height,
            .descent = -descent,
            .ascent = ascent,
        };

        size_t font_id = font_id_to_native.size();
        font_postscript_name_to_id.emplace(font_name, font_id);
        font_id_to_native.emplace_back(ct_font);
        font_id_to_metrics.emplace_back(std::move(metrics));
    }
    return font_postscript_name_to_id.at(font_name);
}

ScopedCFTypeRef<CTLineRef> FontRasterizer::impl::createCTLine(size_t font_id,
                                                              std::string_view str8) {
    CTFontRef ct_font = font_id_to_native[font_id].get();

    ScopedCFTypeRef<CFStringRef> text_string{CFStringCreateWithBytesNoCopy(
        kCFAllocatorDefault, (const uint8_t*)str8.data(), str8.length(), kCFStringEncodingUTF8,
        false, kCFAllocatorNull)};

    ScopedCFTypeRef<CFMutableDictionaryRef> attr{CFDictionaryCreateMutable(
        kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks)};
    CFDictionaryAddValue(attr.get(), kCTFontAttributeName, ct_font);

    ScopedCFTypeRef<CFAttributedStringRef> attr_string{
        CFAttributedStringCreate(kCFAllocatorDefault, text_string.get(), attr.get())};

    return CTLineCreateWithAttributedString(attr_string.get());
}

}
