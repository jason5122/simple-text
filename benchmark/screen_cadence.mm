#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>
#import <Foundation/Foundation.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

struct Frame {
    double time = 0.0;
    double x = 0.0;
    bool found = false;
};

double quantile(std::vector<double> values, double q) {
    std::sort(values.begin(), values.end());
    const double index = q * static_cast<double>(values.size() - 1);
    const size_t low = static_cast<size_t>(std::floor(index));
    const size_t high = static_cast<size_t>(std::ceil(index));
    const double fraction = index - static_cast<double>(low);
    return values[low] * (1.0 - fraction) + values[high] * fraction;
}

void print_distribution(const char* name, const char* unit, const std::vector<double>& values) {
    if (values.empty()) {
        std::printf("%s n=0\n", name);
        return;
    }
    std::printf("%s n=%zu p50=%.3f%s p95=%.3f%s p99=%.3f%s max=%.3f%s\n", name, values.size(),
                quantile(values, 0.50), unit, quantile(values, 0.95), unit, quantile(values, 0.99),
                unit, *std::max_element(values.begin(), values.end()), unit);
}

Frame locate_bar(CMSampleBufferRef sample) {
    Frame frame;
    frame.time = CMTimeGetSeconds(CMSampleBufferGetPresentationTimeStamp(sample));
    CVPixelBufferRef pixels = CMSampleBufferGetImageBuffer(sample);
    if (!pixels) {
        return frame;
    }

    CVPixelBufferLockBaseAddress(pixels, kCVPixelBufferLock_ReadOnly);
    const int width = static_cast<int>(CVPixelBufferGetWidth(pixels));
    const int height = static_cast<int>(CVPixelBufferGetHeight(pixels));
    const size_t stride = CVPixelBufferGetBytesPerRow(pixels);
    const auto* bytes = static_cast<const uint8_t*>(CVPixelBufferGetBaseAddress(pixels));
    int min_x = width;
    int max_x = -1;
    int matches = 0;
    // The benchmark bar is centered vertically. Sampling every other row keeps this probe cheap
    // enough not to perturb the app it is measuring.
    for (int y = height / 3; y < height * 2 / 3; y += 2) {
        const uint8_t* row = bytes + static_cast<size_t>(y) * stride;
        for (int x = 0; x < width; ++x) {
            const uint8_t b = row[static_cast<size_t>(x) * 4];
            const uint8_t g = row[static_cast<size_t>(x) * 4 + 1];
            const uint8_t r = row[static_cast<size_t>(x) * 4 + 2];
            if (g > 190 && b >= 55 && b <= 180 && r < 120 && g > b + 45 && b > r + 20) {
                min_x = std::min(min_x, x);
                max_x = std::max(max_x, x);
                ++matches;
            }
        }
    }
    if (max_x >= min_x && matches >= 20) {
        frame.x = (min_x + max_x + 1) * 0.5;
        frame.found = true;
    }
    CVPixelBufferUnlockBaseAddress(pixels, kCVPixelBufferLock_ReadOnly);
    return frame;
}

}  // namespace

API_AVAILABLE(macos(12.3)) @interface ScreenCadenceProbe
    : NSObject <SCStreamOutput, SCStreamDelegate> {
    SCStream* _stream;
    dispatch_queue_t _queue;
    std::vector<Frame> _frames;
    double _pointsWidth;
    double _speed;
}
- (instancetype)initWithPointsWidth:(double)pointsWidth speed:(double)speed;
- (void)startWindow:(SCWindow*)window duration:(double)duration;
@end

@implementation ScreenCadenceProbe

- (instancetype)initWithPointsWidth:(double)pointsWidth speed:(double)speed {
    self = [super init];
    if (self) {
        _pointsWidth = pointsWidth;
        _speed = speed;
        _queue = dispatch_queue_create("px.screen-cadence", DISPATCH_QUEUE_SERIAL);
    }
    return self;
}

- (void)stream:(SCStream*)stream
    didOutputSampleBuffer:(CMSampleBufferRef)sample
                   ofType:(SCStreamOutputType)type {
    if (type == SCStreamOutputTypeScreen && CMSampleBufferIsValid(sample)) {
        _frames.push_back(locate_bar(sample));
    }
}

- (void)stream:(SCStream*)stream didStopWithError:(NSError*)error {
    std::fprintf(stderr, "screen_cadence: stream stopped: %s\n",
                 error.localizedDescription.UTF8String);
}

- (void)startWindow:(SCWindow*)window duration:(double)duration {
    SCContentFilter* filter = [[SCContentFilter alloc] initWithDesktopIndependentWindow:window];
    SCStreamConfiguration* configuration = [[SCStreamConfiguration alloc] init];
    configuration.width = static_cast<size_t>(std::ceil(window.frame.size.width));
    configuration.height = static_cast<size_t>(std::ceil(window.frame.size.height));
    configuration.minimumFrameInterval = CMTimeMake(1, 120);
    configuration.queueDepth = 8;
    configuration.showsCursor = NO;
    configuration.pixelFormat = kCVPixelFormatType_32BGRA;

    _stream = [[SCStream alloc] initWithFilter:filter configuration:configuration delegate:self];
    NSError* error = nil;
    if (![_stream addStreamOutput:self
                             type:SCStreamOutputTypeScreen
               sampleHandlerQueue:_queue
                            error:&error]) {
        std::fprintf(stderr, "screen_cadence: cannot add output: %s\n",
                     error.localizedDescription.UTF8String);
        std::exit(3);
    }

    [_stream startCaptureWithCompletionHandler:^(NSError* startError) {
      if (startError) {
          std::fprintf(stderr, "screen_cadence: cannot start capture: %s\n",
                       startError.localizedDescription.UTF8String);
          std::exit(3);
      }
      std::printf("capture_started window_id=%u requested_fps=120\n", window.windowID);
      dispatch_after(
          dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(duration * NSEC_PER_SEC)),
          dispatch_get_main_queue(), ^{
            [self->_stream stopCaptureWithCompletionHandler:^(NSError* stopError) {
              dispatch_sync(self->_queue, ^{
                            });
              if (stopError) {
                  std::fprintf(stderr, "screen_cadence: stop failed: %s\n",
                               stopError.localizedDescription.UTF8String);
              }

              std::vector<Frame> found;
              for (const Frame& frame : self->_frames) {
                  if (frame.found) {
                      found.push_back(frame);
                  }
              }
              std::vector<double> intervals;
              std::vector<double> step_errors;
              size_t stalls = 0;
              const double pixelsPerPoint = configuration.width / self->_pointsWidth;
              for (size_t index = 1; index < found.size(); ++index) {
                  const double dt = found[index].time - found[index - 1].time;
                  if (dt <= 0.0) {
                      continue;
                  }
                  intervals.push_back(dt * 1000.0);
                  const double actual = (found[index].x - found[index - 1].x) / pixelsPerPoint;
                  const double expected = self->_speed * dt;
                  step_errors.push_back(std::abs(actual - expected));
                  if (expected > 0.25 && actual < expected * 0.25) {
                      ++stalls;
                  }
              }
              std::printf("benchmark=screen_cadence captured=%zu tracked=%zu stalls=%zu\n",
                          self->_frames.size(), found.size(), stalls);
              print_distribution("capture_interval", "ms", intervals);
              print_distribution("visible_step_error", "pt", step_errors);
              std::exit(found.size() >= 10 ? 0 : 4);
            }];
          });
    }];
}

@end

API_AVAILABLE(macos(12.3))
void run_capture(pid_t pid, double duration, double pointsWidth, double speed) {
    [SCShareableContent
        getShareableContentExcludingDesktopWindows:YES
                               onScreenWindowsOnly:YES
                                 completionHandler:^(SCShareableContent* content, NSError* error) {
                                   if (error) {
                                       std::fprintf(
                                           stderr,
                                           "screen_cadence: cannot enumerate windows: %s\n",
                                           error.localizedDescription.UTF8String);
                                       std::exit(3);
                                   }
                                   SCWindow* target = nil;
                                   for (SCWindow* window in content.windows) {
                                       if (window.owningApplication.processID == pid) {
                                           target = window;
                                           break;
                                       }
                                   }
                                   if (!target) {
                                       std::fprintf(
                                           stderr,
                                           "screen_cadence: no on-screen window for pid %d\n",
                                           pid);
                                       std::exit(3);
                                   }
                                   ScreenCadenceProbe* probe =
                                       [[ScreenCadenceProbe alloc] initWithPointsWidth:pointsWidth
                                                                                 speed:speed];
                                   [probe startWindow:target duration:duration];
                                 }];
    dispatch_main();
}

int main(int argc, char** argv) {
    if (argc != 5) {
        std::fprintf(stderr,
                     "usage: %s PID DURATION_SECONDS POINTS_WIDTH SPEED_POINTS_PER_SECOND\n",
                     argv[0]);
        return 2;
    }
    const pid_t pid = static_cast<pid_t>(std::strtol(argv[1], nullptr, 10));
    const double duration = std::strtod(argv[2], nullptr);
    const double pointsWidth = std::strtod(argv[3], nullptr);
    const double speed = std::strtod(argv[4], nullptr);
    if (pid <= 0 || duration <= 0.0 || pointsWidth <= 0.0 || speed <= 0.0) {
        return 2;
    }

    std::setvbuf(stdout, nullptr, _IONBF, 0);
    if (@available(macOS 12.3, *)) {
        run_capture(pid, duration, pointsWidth, speed);
    } else {
        std::fprintf(stderr, "screen_cadence requires macOS 12.3 or later\n");
        return 3;
    }
}
