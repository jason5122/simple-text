#include "base/apple/scoped_cftyperef.h"
#include "base/apple/scoped_cgtyperef.h"
#include "base/strings/sys_string_conversions.h"
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#include <ImageIO/ImageIO.h>
#include <cmath>
#include <cstdint>
#include <spdlog/spdlog.h>
#include <string_view>
#include <unordered_map>
#include <vector>

using base::apple::ScopedCFTypeRef;
using base::apple::ScopedCGColor;
using base::apple::ScopedCGColorSpace;
using base::apple::ScopedCGContext;
using base::apple::ScopedCGImage;

namespace {

void write_png(std::string_view path, CGImageRef image) {
    auto cf_path = base::sys_utf8_to_cfstring_ref(path);
    auto url = ScopedCFTypeRef<CFURLRef>(CFURLCreateWithFileSystemPath(
        kCFAllocatorDefault, cf_path.get(), kCFURLPOSIXPathStyle, false));

    auto dest = ScopedCFTypeRef<CGImageDestinationRef>(
        CGImageDestinationCreateWithURL(url.get(), CFSTR("public.png"), 1, nullptr));
    CGImageDestinationAddImage(dest.get(), image, nullptr);
    CGImageDestinationFinalize(dest.get());
}

struct Color {
    CGFloat r, g, b;
};

void draw_line(
    CGContextRef ctx, const Color& color, int scale, const CGPoint& p1, const CGPoint& p2) {
    CGContextSetRGBStrokeColor(ctx, color.r, color.g, color.b, 1);
    CGContextSetLineWidth(ctx, 1.0 / scale);
    CGContextMoveToPoint(ctx, p1.x, p1.y);
    CGContextAddLineToPoint(ctx, p2.x, p2.y);
    CGContextStrokePath(ctx);
}

void draw_text(CGContextRef ctx, CTLineRef line, CTFontRef, CGFloat descent) {
    CGContextSetTextMatrix(ctx, CGAffineTransformIdentity);
    CGContextSetTextDrawingMode(ctx, kCGTextFill);

    CGContextSetTextPosition(ctx, 0, descent);
    CTLineDraw(line, ctx);
}

// Identical to `draw_text`!
void draw_text2(CGContextRef ctx, CTLineRef line, CTFontRef font, CGFloat descent) {
    CGContextSetRGBFillColor(ctx, 0, 0, 0, 1);

    CGPoint line_origin = {0, descent};
    CFArrayRef runs = CTLineGetGlyphRuns(line);
    CFIndex run_count = CFArrayGetCount(runs);

    for (CFIndex r = 0; r < run_count; r++) {
        CTRunRef run = (CTRunRef)CFArrayGetValueAtIndex(runs, r);
        CFIndex n = CTRunGetGlyphCount(run);
        if (n <= 0) continue;

        std::vector<CGGlyph> glyphs(n);
        std::vector<CGPoint> positions(n);

        CTRunGetGlyphs(run, CFRangeMake(0, 0), glyphs.data());
        CTRunGetPositions(run, CFRangeMake(0, 0), positions.data());

        // Run attributes can override the font. Use the run font if present.
        CTFontRef run_font = font;
        CFDictionaryRef run_attrs = CTRunGetAttributes(run);
        if (run_attrs) {
            auto v = CFDictionaryGetValue(run_attrs, kCTFontAttributeName);
            if (v) run_font = (CTFontRef)v;
        }

        // Apply line origin to each glyph position (positions are relative to line origin).
        for (CFIndex i = 0; i < n; i++) {
            positions[i].x += line_origin.x;
            positions[i].y += line_origin.y;
        }
        CTFontDrawGlyphs(run_font, glyphs.data(), positions.data(), n, ctx);
    }
}

struct GlyphBitmap {
    size_t width = 0;
    size_t height = 0;
    size_t bytes_per_pixel = 4;
    CGFloat offset_x_user = 0;
    CGFloat offset_y_user = 0;
    // uint8_t phase_x_bucket = 0;
    std::vector<uint8_t> pixels;
};

uint8_t phase_bucket_x(CGFloat origin_x_user, int scale, int buckets) {
    // Phase in device pixels.
    CGFloat x_dev = origin_x_user * (CGFloat)scale;
    CGFloat frac = x_dev - std::floor(x_dev);          // [0,1)
    int b = (int)std::floor(frac * (CGFloat)buckets);  // 0..buckets-1
    if (b < 0) b = 0;
    if (b >= buckets) b = buckets - 1;
    return (uint8_t)b;
}

GlyphBitmap rasterize(
    CTFontRef font, CGGlyph glyph, const CGPoint& origin_user, int scale, int buckets) {
    // Glyph bounds in glyph space (relative to glyph origin).
    CGRect gb =
        CTFontGetBoundingRectsForGlyphs(font, kCTFontOrientationDefault, &glyph, nullptr, 1);

    // Move bounds into destination user space at origin_user.
    CGRect ub = CGRectOffset(gb, origin_user.x, origin_user.y);

    // Pad by 1 device pixel to avoid AA clipping.
    CGFloat pad_user = 1.0 / scale;
    ub = CGRectInset(ub, -pad_user, -pad_user);

    // Compute destination pixel rect.
    int x0 = (int)std::floor(ub.origin.x * scale);
    int y0 = (int)std::floor(ub.origin.y * scale);
    int x1 = (int)std::ceil((ub.origin.x + ub.size.width) * scale);
    int y1 = (int)std::ceil((ub.origin.y + ub.size.height) * scale);

    size_t w = std::max(0, x1 - x0);
    size_t h = std::max(0, y1 - y0);

    if (w == 0 || h == 0) return {};

    size_t bytes_per_pixel = 4;
    size_t bytes_per_row = w * bytes_per_pixel;
    std::vector<uint8_t> pixels(h * bytes_per_row);

    auto cs = ScopedCGColorSpace(CGColorSpaceCreateDeviceRGB());
    auto ctx = ScopedCGContext(
        CGBitmapContextCreate(pixels.data(), w, h, 8, bytes_per_row, cs.get(),
                              kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host));

    // Map glyph drawing into destination user space, but clipped to this bitmap.
    CGContextScaleCTM(ctx.get(), scale, scale);
    CGContextTranslateCTM(ctx.get(), -(CGFloat)x0 / scale, -(CGFloat)y0 / scale);

    // Draw glyph at its real destination position.
    CGContextSetRGBFillColor(ctx.get(), 0, 0, 0, 1);
    CGPoint pos = origin_user;
    CTFontDrawGlyphs(font, &glyph, &pos, 1, ctx.get());

    // Convert crop origin back to user space, then store it as an offset from origin_user.
    CGFloat crop_x_user = (CGFloat)x0 / (CGFloat)scale;
    CGFloat crop_y_user = (CGFloat)y0 / (CGFloat)scale;

    return {
        .width = w,
        .height = h,
        .bytes_per_pixel = bytes_per_pixel,
        .offset_x_user = crop_x_user - origin_user.x,
        .offset_y_user = crop_y_user - origin_user.y,
        // .phase_x_bucket = phase_bucket_x(origin_user.x, scale, buckets),
        .pixels = std::move(pixels),
    };
}

void blit_glyph_bitmap(CGContextRef ctx,
                       const GlyphBitmap& bm,
                       const CGPoint& origin_user,
                       int scale) {
    if (bm.width == 0 || bm.height == 0) return;

    size_t bytes_per_row = bm.width * bm.bytes_per_pixel;
    auto provider = ScopedCFTypeRef<CGDataProviderRef>(CGDataProviderCreateWithData(
        nullptr, bm.pixels.data(), bm.height * bytes_per_row, nullptr));
    auto cs = ScopedCGColorSpace(CGColorSpaceCreateDeviceRGB());
    auto img = ScopedCFTypeRef<CGImageRef>(
        CGImageCreate(bm.width, bm.height, 8, 32, bytes_per_row, cs.get(),
                      kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host, provider.get(),
                      nullptr, false, kCGRenderingIntentDefault));

    // Destination rect in user space.
    CGRect rect = {
        .origin = {origin_user.x + bm.offset_x_user, origin_user.y + bm.offset_y_user},
        .size = {(CGFloat)bm.width / scale, (CGFloat)bm.height / scale},
    };

    CGContextDrawImage(ctx, rect, img.get());
}

// --- Minimal cache keyed by (font pointer, glyph id, x-phase bucket) ---

struct CacheKey {
    CTFontRef font = nullptr;
    CGGlyph glyph = 0;
    uint8_t phase_x_bucket = 0;
};

struct CacheKeyHash {
    size_t operator()(const CacheKey& k) const noexcept {
        size_t h = std::hash<const void*>()((const void*)k.font);
        h ^= (size_t)k.glyph + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        h ^= (size_t)k.phase_x_bucket + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
        return h;
    }
};

struct CacheKeyEq {
    bool operator()(const CacheKey& a, const CacheKey& b) const noexcept {
        return a.font == b.font && a.glyph == b.glyph && a.phase_x_bucket == b.phase_x_bucket;
    }
};

std::unordered_map<CacheKey, GlyphBitmap, CacheKeyHash, CacheKeyEq> g_cache;

const GlyphBitmap& get_cached_glyph(
    CTFontRef font, CGGlyph glyph, const CGPoint& origin_user, int scale, int buckets) {
    CacheKey key = {font, glyph, phase_bucket_x(origin_user.x, scale, buckets)};
    auto it = g_cache.find(key);
    if (it != g_cache.end()) return it->second;

    GlyphBitmap bm = rasterize(font, glyph, origin_user, scale, buckets);
    // bm.phase_x_bucket = key.phase_x_bucket;

    auto [ins_it, _] = g_cache.emplace(key, std::move(bm));
    return ins_it->second;
}

// Uses cached glyph bitmaps. With buckets=1 you will see mismatches; higher buckets converge.
void draw_text3(CGContextRef ctx, CTLineRef line, CTFontRef font, CGFloat descent) {
    CGContextSetRGBFillColor(ctx, 0, 0, 0, 1);

    CGPoint line_origin = {0, descent};
    CFArrayRef runs = CTLineGetGlyphRuns(line);
    CFIndex run_count = CFArrayGetCount(runs);

    constexpr int scale = 2;
    constexpr int buckets = 4;

    for (CFIndex r = 0; r < run_count; r++) {
        CTRunRef run = (CTRunRef)CFArrayGetValueAtIndex(runs, r);
        CFIndex n = CTRunGetGlyphCount(run);
        if (n <= 0) continue;

        std::vector<CGGlyph> glyphs(n);
        std::vector<CGPoint> positions(n);

        CTRunGetGlyphs(run, CFRangeMake(0, 0), glyphs.data());
        CTRunGetPositions(run, CFRangeMake(0, 0), positions.data());

        // Run attributes can override the font. Use the run font if present.
        CTFontRef run_font = font;
        CFDictionaryRef run_attrs = CTRunGetAttributes(run);
        if (run_attrs) {
            auto v = CFDictionaryGetValue(run_attrs, kCTFontAttributeName);
            if (v) run_font = (CTFontRef)v;
        }

        for (CFIndex i = 0; i < n; i++) {
            auto pos = positions[i];
            auto glyph = glyphs[i];
            pos.x += line_origin.x;
            pos.y += line_origin.y;

            const GlyphBitmap& bm = get_cached_glyph(run_font, glyph, pos, scale, buckets);
            blit_glyph_bitmap(ctx, bm, pos, scale);
        }
    }
}

}  // namespace

int main() {
    auto text =
        base::sys_utf8_to_cfstring_ref("Sphinx of black quartz, judge my vow 😀. 0123456789");
    auto family = base::sys_utf8_to_cfstring_ref("Source Code Pro");
    CGFloat font_size = 16;
    const char* out_path = "out.png";

    constexpr size_t width = 1000;
    constexpr size_t height = 200;
    constexpr int scale = 2;

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

    auto font = ScopedCFTypeRef<CTFontRef>(CTFontCreateWithName(family.get(), font_size, nullptr));
    const void* keys[] = {kCTFontAttributeName};
    const void* vals[] = {font.get()};
    auto attrs = ScopedCFTypeRef<CFDictionaryRef>(
        CFDictionaryCreate(kCFAllocatorDefault, keys, vals, 1, &kCFTypeDictionaryKeyCallBacks,
                           &kCFTypeDictionaryValueCallBacks));
    auto as = ScopedCFTypeRef<CFAttributedStringRef>(
        CFAttributedStringCreate(kCFAllocatorDefault, text.get(), attrs.get()));
    auto line = ScopedCFTypeRef<CTLineRef>(CTLineCreateWithAttributedString(as.get()));

    CGFloat ascent = 0, descent = 0, leading = 0;
    double line_width = CTLineGetTypographicBounds(line.get(), &ascent, &descent, &leading);

    // draw_text(ctx.get(), line.get(), font.get(), descent);
    // draw_text2(ctx.get(), line.get(), font.get(), descent);
    draw_text3(ctx.get(), line.get(), font.get(), descent);

    // Debug.
    auto snap = [](CGFloat y) { return (std::floor(y * scale) + 0.5) / scale; };
    double w = std::ceil(line_width);
    draw_line(ctx.get(), {1, 0, 0}, scale, {0, snap(descent)}, {w, snap(descent)});
    draw_line(ctx.get(), {0, 1, 0}, scale, {0, snap(ascent)}, {w, snap(ascent)});
    draw_line(ctx.get(), {0, 0, 1}, scale, {0, snap(0)}, {w, snap(0)});

    auto img = ScopedCGImage(CGBitmapContextCreateImage(ctx.get()));
    write_png(out_path, img.get());
}
