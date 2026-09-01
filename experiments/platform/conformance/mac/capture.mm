#include "experiments/platform/conformance/capture.h"
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
    CGImageRef image_a = (CGImageRef)a, image_b = (CGImageRef)b;
    if (!image_a || !image_b) return false;
    if (CGImageGetWidth(image_a) != CGImageGetWidth(image_b) ||
        CGImageGetHeight(image_a) != CGImageGetHeight(image_b)) {
        return false;
    }
    CFDataRef data_a = CGDataProviderCopyData(CGImageGetDataProvider(image_a));
    CFDataRef data_b = CGDataProviderCopyData(CGImageGetDataProvider(image_b));
    bool equal = data_a && data_b && CFDataGetLength(data_a) == CFDataGetLength(data_b) &&
                 memcmp(CFDataGetBytePtr(data_a), CFDataGetBytePtr(data_b),
                        static_cast<size_t>(CFDataGetLength(data_a))) == 0;
    if (data_a) CFRelease(data_a);
    if (data_b) CFRelease(data_b);
    return equal;
}

}  // namespace

namespace capture {

Frame capture_frame(WindowId window_id, Crop crop) {
    CGImageRef full = copy_window_image(static_cast<uint32_t>(window_id));
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
    CFStringRef path = CFStringCreateWithCString(nullptr, out_path, kCFStringEncodingUTF8);
    CFURLRef url = CFURLCreateWithFileSystemPath(nullptr, path, kCFURLPOSIXPathStyle, false);
    CFRelease(path);
    CGImageDestinationRef destination =
        CGImageDestinationCreateWithURL(url, CFSTR("public.png"), 1, nullptr);
    CFRelease(url);
    if (!destination) return false;
    CGImageDestinationAddImage(destination, (CGImageRef)frame, nullptr);
    bool ok = CGImageDestinationFinalize(destination);
    CFRelease(destination);
    return ok;
}

void pump(double seconds) {
    [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
                             beforeDate:[NSDate dateWithTimeIntervalSinceNow:seconds]];
}

Frame wait_settled(WindowId window_id, Crop crop, Frame baseline) {
    // Two consecutive matching captures are the minimum possible proof of settledness. A 2ms poll
    // interval is the measured knee between added latency and checking faster than ST repaints.
    constexpr double kPollInterval = 0.002;
    constexpr int kMaxFrames = 1000;
    constexpr double kGraceSeconds = 0.2;
    const auto start = std::chrono::steady_clock::now();
    Frame last = nullptr;
    Frame result = nullptr;
    int i;
    for (i = 0; i < kMaxFrames && !result; i++) {
        pump(kPollInterval);
        Frame current = capture_frame(window_id, crop);
        if (!current) continue;
        const bool changed = !baseline || !frames_equal(current, baseline);
        const bool stable = last && frames_equal(current, last);
        const bool graced =
            stable &&
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() >=
                kGraceSeconds;
        if ((changed && stable) || graced) {
            result = current;
        } else {
            release_frame(last);
            last = current;
        }
    }
    if (getenv("RZ_DEBUG_ITERS")) fprintf(stderr, "  iters=%d\n", i);
    if (result) {
        release_frame(last);
    } else {
        result = last;
    }
    return result;
}

// Frontmost normal (layer 0) on-screen window matching `owner_name` (if non-nil) and `owner_pid`
// (if non-zero). CGWindowListCopyWindowInfo returns windows front-to-back, so the first match
// wins.
static WindowId find_window_matching(NSString* owner_name, int owner_pid) {
    CFArrayRef windows = CGWindowListCopyWindowInfo(
        kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements, kCGNullWindowID);
    if (!windows) return 0;
    uint32_t found = 0;
    for (NSDictionary* info in (__bridge NSArray*)windows) {
        if ([info[(__bridge NSString*)kCGWindowLayer] intValue] != 0) continue;
        if (owner_name &&
            ![info[(__bridge NSString*)kCGWindowOwnerName] isEqualToString:owner_name]) {
            continue;
        }
        if (owner_pid && [info[(__bridge NSString*)kCGWindowOwnerPID] intValue] != owner_pid) {
            continue;
        }
        found = [info[(__bridge NSString*)kCGWindowNumber] unsignedIntValue];
        break;
    }
    CFRelease(windows);
    return found;
}

WindowId find_window(const char* owner) {
    return find_window_matching([NSString stringWithUTF8String:owner], 0);
}

WindowId find_window_for_pid(int pid) { return find_window_matching(nil, pid); }

}  // namespace capture
