#include "experiments/rasterizer/mac/capture.h"
#import <AppKit/AppKit.h>
#import <CoreGraphics/CoreGraphics.h>
#import <ImageIO/ImageIO.h>
#include <chrono>
#include <cstring>

// Private SkyLight API: captures a list of windows' backing stores as CGImages. There is no public
// header, so declare the two symbols we use. Chosen over the public CGWindowListCreateImage (which
// is deprecated on recent macOS) for speed.
extern "C" {
typedef int CGSConnectionID;
CGSConnectionID CGSMainConnectionID(void);
CFArrayRef CGSHWCaptureWindowList(CGSConnectionID cid,
                                  const CGWindowID* window_list,
                                  uint32_t window_count,
                                  uint32_t options);
}

namespace {

// Ignore the window's rounded-corner clip so we get the full rectangular backing store. Leaving
// the "nominal resolution" bit unset yields a device-pixel (2x on Retina) image, matching the
// screenshots we diff against.
constexpr uint32_t kIgnoreGlobalClipShape = 0x0800;

CGImageRef copy_window_image(uint32_t window_id) {
    CGWindowID ids[1] = {window_id};
    CFArrayRef list =
        CGSHWCaptureWindowList(CGSMainConnectionID(), ids, 1, kIgnoreGlobalClipShape);
    if (!list) return nullptr;
    CGImageRef image = nullptr;
    if (CFArrayGetCount(list) > 0) {
        image = (CGImageRef)CFArrayGetValueAtIndex(list, 0);
        if (image) CGImageRetain(image);
    }
    CFRelease(list);
    return image;
}

// Exact pixel comparison of two captured frames (the settle loop's stability/change test).
bool frames_equal(void* a, void* b) {
    CGImageRef ia = (CGImageRef)a, ib = (CGImageRef)b;
    if (!ia || !ib) return false;
    if (CGImageGetWidth(ia) != CGImageGetWidth(ib) || CGImageGetHeight(ia) != CGImageGetHeight(ib))
        return false;
    CFDataRef da = CGDataProviderCopyData(CGImageGetDataProvider(ia));
    CFDataRef db = CGDataProviderCopyData(CGImageGetDataProvider(ib));
    bool eq = da && db && CFDataGetLength(da) == CFDataGetLength(db) &&
              memcmp(CFDataGetBytePtr(da), CFDataGetBytePtr(db),
                     static_cast<size_t>(CFDataGetLength(da))) == 0;
    if (da) CFRelease(da);
    if (db) CFRelease(db);
    return eq;
}

}  // namespace

namespace capture {

Frame capture_frame(uint32_t window_id, Crop crop) {
    CGImageRef full = copy_window_image(window_id);
    if (!full || crop.w <= 0 || crop.h <= 0) return full;
    CGImageRef cropped =
        CGImageCreateWithImageInRect(full, CGRectMake(crop.x, crop.y, crop.w, crop.h));
    CGImageRelease(full);
    return cropped;
}

void release_frame(Frame frame) {
    if (frame) CGImageRelease((CGImageRef)frame);
}

bool frame_to_png(Frame frame, const char* out_path) {
    if (!frame) return false;
    CFStringRef cfpath = CFStringCreateWithCString(nullptr, out_path, kCFStringEncodingUTF8);
    CFURLRef url = CFURLCreateWithFileSystemPath(nullptr, cfpath, kCFURLPOSIXPathStyle, false);
    CFRelease(cfpath);
    CGImageDestinationRef dst =
        CGImageDestinationCreateWithURL(url, CFSTR("public.png"), 1, nullptr);
    CFRelease(url);
    if (!dst) return false;
    CGImageDestinationAddImage(dst, (CGImageRef)frame, nullptr);
    bool ok = CGImageDestinationFinalize(dst);
    CFRelease(dst);
    return ok;
}

void pump(double seconds) {
    [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
                             beforeDate:[NSDate dateWithTimeIntervalSinceNow:seconds]];
}

Frame wait_settled(uint32_t window_id, Crop crop, Frame baseline) {
    // Two consecutive matching captures are the minimum possible proof of settledness (one to see
    // the change, one to confirm it isn't still transitioning), and that's what real shots hit
    // almost every time (measured: ~85% at this interval, the rest need a 3rd). So the poll interval
    // is pure added latency on the common path -- tightened from an earlier 8ms to 2ms recovers
    // ~22% of per-shot time. Going tighter (0.5ms) saves little more and starts costing extra
    // iterations instead (polling faster than ST's own repaint completes), so this is the knee.
    constexpr double kPollInterval = 0.002;
    constexpr int kMaxFrames = 1000;  // ~2s hard ceiling at kPollInterval
    constexpr double kGraceSeconds =
        0.2;  // once the crop is stable this long without changing from
              // the baseline, take it -- the new render is genuinely
              // identical (e.g. a crop on empty space, or two shots that
              // render the same there). subl returned before this call,
              // so ST has already repainted; a stable frame is the real
              // one, not a stale pre-repaint frame. Without this the loop
              // spins to kMaxFrames whenever a shot doesn't change the crop.
    const auto start = std::chrono::steady_clock::now();
    Frame last = nullptr;
    Frame result = nullptr;
    int i;
    for (i = 0; i < kMaxFrames && !result; i++) {
        pump(kPollInterval);
        Frame cur = capture_frame(window_id, crop);
        if (!cur) continue;
        const bool changed = !baseline || !frames_equal(cur, baseline);
        const bool stable = last && frames_equal(cur, last);
        const bool graced =
            stable &&
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() >=
                kGraceSeconds;
        if ((changed && stable) || graced) {
            result = cur;  // changed from the previous shot and stable, or stable-but-unchanged
                           // past grace
        } else {
            release_frame(last);
            last = cur;
        }
    }
    if (getenv("RZ_DEBUG_ITERS")) fprintf(stderr, "  iters=%d\n", i);
    if (result) release_frame(last);
    else result = last;  // best effort if it never settled
    return result;
}

// Frontmost normal (layer 0) on-screen window matching `owner_name` (if non-nil) and `owner_pid`
// (if non-zero). CGWindowListCopyWindowInfo returns windows front-to-back, so the first match
// wins.
static uint32_t find_window_matching(NSString* owner_name, int owner_pid) {
    CFArrayRef windows = CGWindowListCopyWindowInfo(
        kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements, kCGNullWindowID);
    if (!windows) return 0;
    uint32_t found = 0;
    for (NSDictionary* info in (__bridge NSArray*)windows) {
        if ([info[(__bridge NSString*)kCGWindowLayer] intValue] != 0) continue;
        if (owner_name &&
            ![info[(__bridge NSString*)kCGWindowOwnerName] isEqualToString:owner_name])
            continue;
        if (owner_pid && [info[(__bridge NSString*)kCGWindowOwnerPID] intValue] != owner_pid)
            continue;
        found = [info[(__bridge NSString*)kCGWindowNumber] unsignedIntValue];
        break;
    }
    CFRelease(windows);
    return found;
}

uint32_t find_window(const char* owner) {
    return find_window_matching([NSString stringWithUTF8String:owner], 0);
}

uint32_t find_window_for_pid(int pid) { return find_window_matching(nil, pid); }

}  // namespace capture
