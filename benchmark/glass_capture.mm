// Records what the display actually shows of one application's window, frame by frame, using
// ScreenCaptureKit at the panel's refresh rate. This is the observer that treats every
// application the same: it does not care whether the window draws with Metal, OpenGL or Core
// Graphics, or how it paces itself; it only sees the pixels the window server put on the glass
// and when. Pair it with drive_scroll, which posts the same recorded gesture into each window.
//
//   glass_capture --pid PID --seconds N --out frames.tsv [--strip-x PT --strip-width PT]
//                 [--inset-top PT --inset-bottom PT] [--display]
//
// A vertical strip of the window (default: 240 pt wide starting 120 pt in from the left, the
// full height less the insets, which should exclude tab bars and status bars: rows that never
// move vote for a shift of zero) is captured; for each delivered frame the strip's rows are
// hashed and the vertical shift against the previous frame is the one most rows vote for. Output,
// tab separated: display_time (seconds, mach clock), status (complete/idle), shift_px (content
// motion since the previous complete frame, positive = content moved up), dirty (number of dirty
// rects reported). ScreenCaptureKit delivers a frame only when the window's content changed, so
// the gaps between complete frames are the shown cadence. Needs Screen Recording permission.

#import <AppKit/AppKit.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>
#import <Foundation/Foundation.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mach/mach_time.h>
#include <unordered_map>
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

// Vertical shift of `current` against `previous`, each a per-row hash of the strip's pixels, in
// rows. Rows of text have near-identical ink totals line after line, so sums alias by whole lines;
// hashes of the actual pixels do not, since neighbouring lines differ in content. Every row of the
// new frame votes for the shift that lands it on an identical row of the old frame; rows whose
// hash is the most common one (background) do not vote. Positive means the content moved up.
struct shift_estimate {
    int shift = 0;
    int votes = 0;  // 0 means no row of the new frame was found in the old one
};

shift_estimate row_shift(const std::vector<uint64_t>& previous,
                         const std::vector<uint64_t>& current,
                         int range) {
    std::unordered_map<uint64_t, int> frequency;
    for (uint64_t h : previous) {
        ++frequency[h];
    }
    uint64_t background = 0;
    int background_count = 0;
    for (const auto& [h, count] : frequency) {
        if (count > background_count) {
            background_count = count;
            background = h;
        }
    }
    std::unordered_map<uint64_t, std::vector<int>> rows_by_hash;
    for (size_t r = 0; r < previous.size(); ++r) {
        if (previous[r] != background) {
            rows_by_hash[previous[r]].push_back(static_cast<int>(r));
        }
    }
    std::unordered_map<int, int> votes;
    for (size_t r = 0; r < current.size(); ++r) {
        if (current[r] == background) {
            continue;
        }
        auto it = rows_by_hash.find(current[r]);
        if (it == rows_by_hash.end() || it->second.size() > 8) {
            continue;  // unmatched, or a row pattern too common to place
        }
        for (int old_row : it->second) {
            const int shift = old_row - static_cast<int>(r);
            if (std::abs(shift) <= range) {
                ++votes[shift];
            }
        }
    }
    shift_estimate best;
    for (const auto& [shift, count] : votes) {
        if (count > best.votes ||
            (count == best.votes && std::abs(shift) < std::abs(best.shift))) {
            best.votes = count;
            best.shift = shift;
        }
    }
    return best;
}

}  // namespace

API_AVAILABLE(macos(12.3))
@interface GlassOutput : NSObject <SCStreamOutput>
@property(nonatomic) FILE* file;
@property(nonatomic) int range;
@property(nonatomic) size_t frames;
@end

@implementation GlassOutput {
    std::vector<uint64_t> _previous;
    bool _havePrevious;
}

@synthesize file = _file;
@synthesize range = _range;
@synthesize frames = _frames;

- (void)stream:(SCStream*)stream
    didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
                   ofType:(SCStreamOutputType)type {
    if (type != SCStreamOutputTypeScreen) {
        return;
    }
    CFArrayRef attachments = CMSampleBufferGetSampleAttachmentsArray(sampleBuffer, false);
    NSDictionary* info = attachments && CFArrayGetCount(attachments) > 0
                             ? (__bridge NSDictionary*)CFArrayGetValueAtIndex(attachments, 0)
                             : nil;
    NSNumber* statusNumber = info[SCStreamFrameInfoStatus];
    const SCFrameStatus status =
        statusNumber ? static_cast<SCFrameStatus>(statusNumber.integerValue) : SCFrameStatusComplete;
    NSNumber* displayTime = info[SCStreamFrameInfoDisplayTime];
    NSArray* dirty = info[SCStreamFrameInfoDirtyRects];
    const double pts = CMTimeGetSeconds(CMSampleBufferGetPresentationTimeStamp(sampleBuffer));
    const double shownAt = displayTime ? mach_seconds(displayTime.unsignedLongLongValue) : pts;
    if (status != SCFrameStatusComplete) {
        std::fprintf(self.file, "%.6f\t%.6f\tidle\t0\t0\t0\n", shownAt, pts);
        return;
    }
    CVImageBufferRef image = CMSampleBufferGetImageBuffer(sampleBuffer);
    if (!image) {
        return;
    }
    CVPixelBufferLockBaseAddress(image, kCVPixelBufferLock_ReadOnly);
    const size_t width = CVPixelBufferGetWidth(image);
    const size_t height = CVPixelBufferGetHeight(image);
    const size_t stride = CVPixelBufferGetBytesPerRow(image);
    const uint8_t* base = static_cast<const uint8_t*>(CVPixelBufferGetBaseAddress(image));
    std::vector<uint64_t> profile(height, 0);
    if (base) {
        for (size_t y = 0; y < height; ++y) {
            const uint8_t* row = base + y * stride;
            // FNV-1a over the row's green channel; BGRA, and green stands in for luminance.
            uint64_t h = 1469598103934665603ULL;
            for (size_t x = 0; x < width; ++x) {
                h ^= row[x * 4 + 1];
                h *= 1099511628211ULL;
            }
            profile[y] = h;
        }
    }
    CVPixelBufferUnlockBaseAddress(image, kCVPixelBufferLock_ReadOnly);
    shift_estimate shift;
    if (_havePrevious) {
        shift = row_shift(_previous, profile, self.range);
    }
    _previous.swap(profile);
    _havePrevious = true;
    ++self.frames;
    std::fprintf(self.file, "%.6f\t%.6f\tcomplete\t%d\t%d\t%lu\n", shownAt, pts, shift.shift,
                 shift.votes, static_cast<unsigned long>(dirty.count));
}

@end

int main(int argc, char** argv) {
    pid_t pid = 0;
    double seconds = 12.0;
    const char* outPath = nullptr;
    double stripX = 120.0;
    double stripWidth = 240.0;
    double insetTop = 0.0;
    double insetBottom = 0.0;
    bool captureDisplay = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--pid") == 0 && i + 1 < argc) {
            pid = static_cast<pid_t>(std::atoi(argv[++i]));
        } else if (std::strcmp(argv[i], "--seconds") == 0 && i + 1 < argc) {
            seconds = std::atof(argv[++i]);
        } else if (std::strcmp(argv[i], "--out") == 0 && i + 1 < argc) {
            outPath = argv[++i];
        } else if (std::strcmp(argv[i], "--strip-x") == 0 && i + 1 < argc) {
            stripX = std::atof(argv[++i]);
        } else if (std::strcmp(argv[i], "--strip-width") == 0 && i + 1 < argc) {
            stripWidth = std::atof(argv[++i]);
        } else if (std::strcmp(argv[i], "--inset-top") == 0 && i + 1 < argc) {
            insetTop = std::atof(argv[++i]);
        } else if (std::strcmp(argv[i], "--inset-bottom") == 0 && i + 1 < argc) {
            insetBottom = std::atof(argv[++i]);
        } else if (std::strcmp(argv[i], "--display") == 0) {
            captureDisplay = true;
        }
    }
    if (!pid || !outPath) {
        std::fprintf(stderr, "usage: glass_capture --pid PID --seconds N --out FRAMES.tsv "
                             "[--strip-x PT --strip-width PT]\n");
        return 2;
    }
    if (@available(macOS 14.0, *)) {
    } else {
        std::fprintf(stderr, "glass_capture: needs macOS 14\n");
        return 2;
    }
    // ScreenCaptureKit needs a window server connection, which only an application has.
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyProhibited];
    FILE* file = std::fopen(outPath, "w");
    if (!file) {
        std::fprintf(stderr, "glass_capture: cannot write %s\n", outPath);
        return 3;
    }
    std::fprintf(file,
                 "# px-glass-capture-v1\n# display_time\tpts\tstatus\tshift_px\tvotes\tdirty\n");

    __block int result = 0;
    dispatch_semaphore_t done = dispatch_semaphore_create(0);
    if (@available(macOS 14.0, *)) {
        [SCShareableContent getShareableContentWithCompletionHandler:^(SCShareableContent* content,
                                                                       NSError* error) {
          if (error || !content) {
              std::fprintf(stderr, "glass_capture: no shareable content (%s); grant Screen "
                                   "Recording to this terminal\n",
                           error.localizedDescription.UTF8String);
              result = 5;
              dispatch_semaphore_signal(done);
              return;
          }
          SCWindow* best = nil;
          for (SCWindow* window in content.windows) {
              if (window.owningApplication.processID != pid || !window.onScreen ||
                  window.windowLayer != 0) {
                  continue;
              }
              if (!best || window.frame.size.width * window.frame.size.height >
                               best.frame.size.width * best.frame.size.height) {
                  best = window;
              }
          }
          if (!best) {
              std::fprintf(stderr, "glass_capture: pid %d has no on-screen window\n", pid);
              result = 4;
              dispatch_semaphore_signal(done);
              return;
          }
          SCContentFilter* filter = nil;
          const CGRect frame = best.frame;
          const CGFloat x = std::min(stripX, frame.size.width - 10.0);
          const CGFloat w = std::min(stripWidth, frame.size.width - x);
          const CGFloat y = std::min(insetTop, frame.size.height - 10.0);
          const CGFloat h = std::max(10.0, frame.size.height - y - insetBottom);
          CGRect source = CGRectMake(x, y, w, h);
          if (captureDisplay) {
              // The display that holds the window, with the strip addressed in display points.
              SCDisplay* display = nil;
              for (SCDisplay* candidate in content.displays) {
                  if (CGRectContainsPoint(candidate.frame,
                                          CGPointMake(CGRectGetMidX(frame), CGRectGetMidY(frame)))) {
                      display = candidate;
                  }
              }
              if (!display) {
                  display = content.displays.firstObject;
              }
              filter = [[SCContentFilter alloc] initWithDisplay:display excludingWindows:@[]];
              source = CGRectMake(frame.origin.x - display.frame.origin.x + x,
                                  frame.origin.y - display.frame.origin.y + y, w, h);
          } else {
              filter = [[SCContentFilter alloc] initWithDesktopIndependentWindow:best];
          }
          SCStreamConfiguration* config = [[SCStreamConfiguration alloc] init];
          const CGFloat scale = filter.pointPixelScale > 0 ? filter.pointPixelScale : 2.0;
          config.sourceRect = source;
          config.width = static_cast<size_t>(w * scale);
          config.height = static_cast<size_t>(h * scale);
          config.minimumFrameInterval = CMTimeMake(1, 120);
          config.queueDepth = 8;
          config.pixelFormat = kCVPixelFormatType_32BGRA;
          config.showsCursor = NO;
          config.capturesAudio = NO;

          GlassOutput* output = [[GlassOutput alloc] init];
          output.file = file;
          // A flick's fastest frame can move half the strip; search the whole height.
          output.range = static_cast<int>(h * scale) - 1;
          SCStream* stream = [[SCStream alloc] initWithFilter:filter
                                                configuration:config
                                                     delegate:nil];
          dispatch_queue_t queue = dispatch_queue_create("glass_capture", DISPATCH_QUEUE_SERIAL);
          NSError* addError = nil;
          [stream addStreamOutput:output
                             type:SCStreamOutputTypeScreen
               sampleHandlerQueue:queue
                            error:&addError];
          std::fprintf(stderr,
                       "glass_capture: window %.0fx%.0f pt, strip x=%.0f w=%.0f y=%.0f h=%.0f, %.1f s\n",
                       frame.size.width, frame.size.height, x, w, y, h, seconds);
          [stream startCaptureWithCompletionHandler:^(NSError* startError) {
            if (startError) {
                std::fprintf(stderr, "glass_capture: cannot start capture: %s\n",
                             startError.localizedDescription.UTF8String);
                result = 5;
                dispatch_semaphore_signal(done);
                return;
            }
            dispatch_after(
                dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(seconds * 1e9)), queue, ^{
                  [stream stopCaptureWithCompletionHandler:^(NSError*) {
                    std::fprintf(stderr, "glass_capture: %zu complete frames\n", output.frames);
                    dispatch_semaphore_signal(done);
                  }];
                });
          }];
        }];
    }
    while (dispatch_semaphore_wait(done, dispatch_time(DISPATCH_TIME_NOW, 50'000'000)) != 0) {
        [[NSRunLoop mainRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
    }
    std::fclose(file);
    return result;
}
