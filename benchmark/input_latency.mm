// Key-to-glass latency of any application's window, for comparing editors on equal terms.
//
// Posts a key press at the HID tap and takes the display time of the next frame in which the
// window's pixels changed, captured from the display with ScreenCaptureKit at the panel's rate.
// The pipeline measured is HID tap -> app -> present -> window server -> glass, the same for every
// app. Preconditions: the target's key window has a focused, empty buffer to type into (or pass
// --new-buffer for Cmd+N), and nothing in the window blinks or animates, since any change after
// the key counts as its response. Needs Screen Recording and Accessibility permission for the
// responsible process, usually the terminal.
//
//   key_to_glass (--app NAME | --pid PID) [--keys 40] [--new-buffer] [--cleanup] [--out
//   trials.tsv]
//
// Compare means over 40 or more keys and more than one run: a single run's trials spread across
// two refreshes of phase, and run-to-run means move by about 2 ms. KEY_TO_GLASS_DEBUG=1 lists
// every changed frame within 300 ms of each key, which tells a late paint from a missed one;
// KEY_TO_GLASS_DISPLAY_FILTER=1 captures the whole screen area instead of the window alone.

#import <AppKit/AppKit.h>
#import <CoreMedia/CoreMedia.h>
#import <QuartzCore/QuartzCore.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <format>
#include <mach/mach_time.h>
#include <mutex>
#include <print>
#include <random>
#include <string>
#include <unistd.h>
#include <vector>

namespace {

double mach_seconds(uint64_t ticks) {
    static mach_timebase_info_data_t tb = [] {
        mach_timebase_info_data_t t{};
        mach_timebase_info(&t);
        return t;
    }();
    return static_cast<double>(ticks) * tb.numer / tb.denom / 1e9;
}

std::mutex g_mutex;
std::vector<double> g_changes;  // display times of frames whose pixels differed from the previous
std::vector<double> g_shown;    // display times of every complete frame (debug)
std::vector<double> g_handler_ms;  // time spent diffing each frame (debug)
int g_frames = 0;
bool g_debug = getenv("KEY_TO_GLASS_DEBUG") != nullptr;

void post_key(CGEventSourceRef source, CGKeyCode code, CGEventFlags flags, const char* text) {
    CGEventRef down = CGEventCreateKeyboardEvent(source, code, true);
    CGEventSetFlags(down, flags);
    if (text) {
        UniChar u = static_cast<UniChar>(text[0]);
        CGEventKeyboardSetUnicodeString(down, 1, &u);
    }
    CGEventPost(kCGHIDEventTap, down);
    CFRelease(down);
    usleep(12'000);
    CGEventRef up = CGEventCreateKeyboardEvent(source, code, false);
    CGEventSetFlags(up, flags);
    CGEventPost(kCGHIDEventTap, up);
    CFRelease(up);
}

// Latency in ms to the first changed frame shown after `since`, or -1 after `timeout_s`.
double wait_change(double since, double timeout_s) {
    double deadline = CACurrentMediaTime() + timeout_s;
    while (CACurrentMediaTime() < deadline) {
        {
            std::lock_guard lock(g_mutex);
            for (double t : g_changes) {
                if (t > since) return (t - since) * 1000.0;
            }
        }
        usleep(500);
    }
    return -1.0;
}

double percentile(std::vector<double> v, double p) {
    if (v.empty()) return 0.0;
    std::sort(v.begin(), v.end());
    return v[static_cast<size_t>(std::lround(p * (v.size() - 1)))];
}

void run_loop_until(bool* flag) {
    while (!*flag)
        [[NSRunLoop mainRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
}

}  // namespace

API_AVAILABLE(macos(14.0))
@interface ChangeOutput : NSObject <SCStreamOutput>
@end

@implementation ChangeOutput {
    std::vector<uint8_t> _previous;
}

// ScreenCaptureKit also delivers frames whose content did not change, so every complete frame is
// compared with the previous one and only a difference counts.
- (void)stream:(SCStream*)stream
    didOutputSampleBuffer:(CMSampleBufferRef)sb
                   ofType:(SCStreamOutputType)type {
    if (type != SCStreamOutputTypeScreen) return;
    CFArrayRef attachments = CMSampleBufferGetSampleAttachmentsArray(sb, false);
    NSDictionary* info = attachments && CFArrayGetCount(attachments) > 0
                             ? (__bridge NSDictionary*)CFArrayGetValueAtIndex(attachments, 0)
                             : nil;
    NSNumber* status = info[SCStreamFrameInfoStatus];
    NSNumber* displayTime = info[SCStreamFrameInfoDisplayTime];
    CVImageBufferRef image = CMSampleBufferGetImageBuffer(sb);
    if ((status && status.integerValue != SCFrameStatusComplete) || !image) return;
    double shown = displayTime ? mach_seconds(displayTime.unsignedLongLongValue)
                               : CMTimeGetSeconds(CMSampleBufferGetPresentationTimeStamp(sb));
    double handler_start = CACurrentMediaTime();

    CVPixelBufferLockBaseAddress(image, kCVPixelBufferLock_ReadOnly);
    size_t width = CVPixelBufferGetWidth(image);
    size_t height = CVPixelBufferGetHeight(image);
    size_t stride = CVPixelBufferGetBytesPerRow(image);
    const uint8_t* base = static_cast<const uint8_t*>(CVPixelBufferGetBaseAddress(image));
    bool changed = _previous.size() != width * height * 4;
    for (size_t y = 0; y < height && !changed; ++y) {
        changed = memcmp(base + y * stride, _previous.data() + y * width * 4, width * 4) != 0;
    }
    if (changed) {
        _previous.resize(width * height * 4);
        for (size_t y = 0; y < height; ++y) {
            memcpy(_previous.data() + y * width * 4, base + y * stride, width * 4);
        }
    }
    CVPixelBufferUnlockBaseAddress(image, kCVPixelBufferLock_ReadOnly);
    std::lock_guard lock(g_mutex);
    g_frames++;
    if (changed) g_changes.push_back(shown);
    if (g_debug) {
        g_shown.push_back(shown);
        g_handler_ms.push_back((CACurrentMediaTime() - handler_start) * 1000.0);
    }
}

@end

namespace {

struct options {
    pid_t pid = 0;
    const char* app = nullptr;
    int keys = 40;
    bool newBuffer = false;
    bool cleanup = false;
    const char* outPath = nullptr;
};

// Everything that touches ScreenCaptureKit, which the deployment target predates.
API_AVAILABLE(macos(14.0))
int run(const options& opt) {
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyProhibited];
    if (!CGPreflightScreenCaptureAccess() || !AXIsProcessTrusted()) {
        std::println(stderr, "key_to_glass: needs Screen Recording and Accessibility permission");
        return 2;
    }

    // The target's largest on-screen window, and the display it is on.
    __block SCWindow* window = nil;
    __block SCDisplay* display = nil;
    __block bool found = false;
    [SCShareableContent getShareableContentWithCompletionHandler:^(SCShareableContent* content,
                                                                   NSError* error) {
      for (SCWindow* w in content.windows) {
          if (!w.onScreen || w.windowLayer != 0) continue;
          bool match =
              opt.pid ? w.owningApplication.processID == opt.pid
                      : [w.owningApplication.applicationName rangeOfString:@(opt.app)].location !=
                            NSNotFound;
          if (!match) continue;
          if (!window || w.frame.size.width * w.frame.size.height >
                             window.frame.size.width * window.frame.size.height) {
              window = w;
          }
      }
      for (SCDisplay* d in content.displays) {
          if (window && CGRectContainsPoint(d.frame, CGPointMake(CGRectGetMidX(window.frame),
                                                                 CGRectGetMidY(window.frame)))) {
              display = d;
          }
      }
      found = true;
    }];
    run_loop_until(&found);
    if (!window || !display) {
        std::println(stderr, "key_to_glass: no on-screen window for the target");
        return 3;
    }
    // The title bar and tab strip are left out: window chrome has its own animations (the
    // capture indicator, traffic-light hover) and an editor's response is never there.
    constexpr double kChromeHeight = 40.0;
    CGRect frame = window.frame;
    frame.origin.y += kChromeHeight;
    frame.size.height -= kChromeHeight;

    CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateCombinedSessionState);
    [[NSRunningApplication
        runningApplicationWithProcessIdentifier:window.owningApplication.processID]
        activateWithOptions:0];
    usleep(500'000);
    if (opt.newBuffer) {
        post_key(source, 45, kCGEventFlagMaskCommand, nullptr);  // Cmd+N
        usleep(800'000);
    }
    // The window's frame, captured from the display at the panel's rate, showing only this window:
    // anything else on the screen, down to the Dock's live preview of a minimized game, would
    // otherwise recomposite the area and its pixel changes would be taken for responses.
    SCContentFilter* filter =
        getenv("KEY_TO_GLASS_DISPLAY_FILTER")
            ? [[SCContentFilter alloc] initWithDisplay:display excludingWindows:@[]]
            : [[SCContentFilter alloc] initWithDisplay:display includingWindows:@[ window ]];
    SCStreamConfiguration* config = [[SCStreamConfiguration alloc] init];
    config.sourceRect =
        CGRectMake(frame.origin.x - display.frame.origin.x,
                   frame.origin.y - display.frame.origin.y, frame.size.width, frame.size.height);
    config.width = static_cast<size_t>(frame.size.width * filter.pointPixelScale);
    config.height = static_cast<size_t>(frame.size.height * filter.pointPixelScale);
    config.minimumFrameInterval = CMTimeMake(1, 120);
    config.queueDepth = 8;
    config.pixelFormat = kCVPixelFormatType_32BGRA;
    config.showsCursor = NO;
    SCStream* stream = [[SCStream alloc] initWithFilter:filter configuration:config delegate:nil];
    ChangeOutput* output = [[ChangeOutput alloc] init];  // the stream only holds it weakly
    [stream addStreamOutput:output
                       type:SCStreamOutputTypeScreen
         sampleHandlerQueue:dispatch_queue_create("key_to_glass", DISPATCH_QUEUE_SERIAL)
                      error:nil];
    __block bool started = false;
    __block bool failed = false;
    [stream startCaptureWithCompletionHandler:^(NSError* error) {
      failed = error != nil;
      started = true;
    }];
    run_loop_until(&started);
    if (failed) {
        std::println(stderr, "key_to_glass: cannot start capture");
        return 4;
    }
    usleep(500'000);

    // One key per trial at a human-like gap, so the phase against the refresh varies.
    std::mt19937 rng(1);
    std::uniform_int_distribution<int> gap(300'000, 500'000);
    std::vector<double> latencies;
    int misses = 0;
    FILE* out = opt.outPath ? fopen(opt.outPath, "w") : nullptr;
    std::vector<double> starts;
    auto debug_trial = [&](int i) {
        // Every changed frame within 300 ms of the key, and the composite cadence around it.
        std::lock_guard lock(g_mutex);
        std::string changes;
        for (double t : g_changes) {
            if (t > starts[i] && t < starts[i] + 0.3) {
                changes += std::format(" +{:.1f}", (t - starts[i]) * 1000.0);
            }
        }
        int composites = 0;
        double largest_gap = 0.0;
        double previous = 0.0;
        for (double t : g_shown) {
            if (t <= starts[i] || t >= starts[i] + 0.3) continue;
            composites++;
            if (previous > 0.0) largest_gap = std::max(largest_gap, (t - previous) * 1000.0);
            previous = t;
        }
        std::println(
            "  trial {:2}: changes at{} ms; {} composites in 300 ms, largest gap {:.1f} ms", i,
            changes, composites, largest_gap);
    };
    for (int i = 0; i < opt.keys; ++i) {
        usleep(gap(rng));
        if (g_debug && i > 0) debug_trial(i - 1);
        double t0 = CACurrentMediaTime();
        starts.push_back(t0);
        post_key(source, 7, 0, "x");
        double ms = wait_change(t0, 0.3);
        if (out) std::println(out, "{}\t{:.2f}", i, ms);
        if (ms < 0) {
            misses++;
        } else {
            latencies.push_back(ms);
        }
    }
    if (out) fclose(out);
    if (g_debug && opt.keys > 0) {
        usleep(400'000);
        debug_trial(opt.keys - 1);
        std::lock_guard lock(g_mutex);
        std::println("  diff per frame: p50 {:.2f} ms, max {:.2f} ms",
                     percentile(g_handler_ms, 0.5), percentile(g_handler_ms, 1.0));
    }
    if (opt.cleanup) {
        post_key(source, 0, kCGEventFlagMaskCommand, nullptr);  // Cmd+A
        usleep(150'000);
        post_key(source, 51, 0, nullptr);  // Delete
    }

    double sum = 0.0;
    for (double ms : latencies) sum += ms;
    // Frames arrive whenever the window server recomposites the captured area, for any reason on
    // the screen; only the ones whose pixels changed take part in the measurement.
    std::println("{}: {} keys shown, {} missed, {} frames composited, {} with changed pixels",
                 window.owningApplication.applicationName.UTF8String, latencies.size(), misses,
                 g_frames, g_changes.size());
    std::println("  mean   {:6.1f} ms", latencies.empty() ? 0.0 : sum / latencies.size());
    std::println("  p10    {:6.1f} ms", percentile(latencies, 0.1));
    std::println("  median {:6.1f} ms", percentile(latencies, 0.5));
    std::println("  p90    {:6.1f} ms", percentile(latencies, 0.9));
    std::println("  min    {:6.1f} ms", percentile(latencies, 0.0));
    std::println("  max    {:6.1f} ms", percentile(latencies, 1.0));
    __block bool stopped = false;
    [stream stopCaptureWithCompletionHandler:^(NSError*) {
      stopped = true;
    }];
    run_loop_until(&stopped);
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    options opt;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--pid") && i + 1 < argc) {
            opt.pid = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--app") && i + 1 < argc) {
            opt.app = argv[++i];
        } else if (!strcmp(argv[i], "--keys") && i + 1 < argc) {
            opt.keys = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--new-buffer")) {
            opt.newBuffer = true;
        } else if (!strcmp(argv[i], "--cleanup")) {
            opt.cleanup = true;
        } else if (!strcmp(argv[i], "--out") && i + 1 < argc) {
            opt.outPath = argv[++i];
        } else {
            std::println(stderr, "usage: key_to_glass (--app NAME | --pid PID) [--keys N] "
                                 "[--new-buffer] [--cleanup] [--out FILE]");
            return 2;
        }
    }
    if (!opt.pid && !opt.app) {
        std::println(stderr, "key_to_glass: --app or --pid is required");
        return 2;
    }

    if (@available(macOS 14.0, *)) {
        return run(opt);
    }
    std::println(stderr, "key_to_glass: needs macOS 14");
    return 2;
}
