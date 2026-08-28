#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <vector>

namespace {

struct FramePosition {
    double time = 0.0;
    double x = 0.0;
    bool found = false;
};

double quantile(std::vector<double> values, double q) {
    if (values.empty()) {
        return 0.0;
    }
    std::sort(values.begin(), values.end());
    const double index = q * static_cast<double>(values.size() - 1);
    const size_t lo = static_cast<size_t>(std::floor(index));
    const size_t hi = static_cast<size_t>(std::ceil(index));
    const double fraction = index - static_cast<double>(lo);
    return values[lo] * (1.0 - fraction) + values[hi] * fraction;
}

void print_distribution(const char* name, const char* unit, const std::vector<double>& values) {
    std::printf("%-22s n=%3zu p50=%8.3f p90=%8.3f p99=%8.3f %s\n", name, values.size(),
                quantile(values, 0.50), quantile(values, 0.90), quantile(values, 0.99), unit);
}

FramePosition locate_bar(CMSampleBufferRef sample, double points_width, double top_points) {
    FramePosition frame;
    frame.time = CMTimeGetSeconds(CMSampleBufferGetPresentationTimeStamp(sample));
    CVPixelBufferRef pixels = CMSampleBufferGetImageBuffer(sample);
    if (!pixels) {
        return frame;
    }

    CVPixelBufferLockBaseAddress(pixels, kCVPixelBufferLock_ReadOnly);
    const int width = static_cast<int>(CVPixelBufferGetWidth(pixels));
    const int height = static_cast<int>(CVPixelBufferGetHeight(pixels));
    const int scan_height = top_points > 0.0
                                ? std::min(height, static_cast<int>(std::ceil(
                                                       top_points * width / points_width)))
                                : height;
    const size_t stride = CVPixelBufferGetBytesPerRow(pixels);
    const auto* bytes = static_cast<const uint8_t*>(CVPixelBufferGetBaseAddress(pixels));

    int min_x = width;
    int max_x = -1;
    int matching_pixels = 0;
    for (int y = 0; y < scan_height; ++y) {
        const uint8_t* row = bytes + static_cast<size_t>(y) * stride;
        for (int x = 0; x < width; ++x) {
            const uint8_t b = row[static_cast<size_t>(x) * 4 + 0];
            const uint8_t g = row[static_cast<size_t>(x) * 4 + 1];
            const uint8_t r = row[static_cast<size_t>(x) * 4 + 2];
            if (g > 190 && b >= 55 && b <= 180 && r < 120 && g > b + 45 && b > r + 20) {
                min_x = std::min(min_x, x);
                max_x = std::max(max_x, x);
                ++matching_pixels;
            }
        }
    }
    if (max_x >= min_x && matching_pixels >= 100) {
        frame.x = (min_x + max_x + 1) * 0.5;
        frame.found = true;
    }
    CVPixelBufferUnlockBaseAddress(pixels, kCVPixelBufferLock_ReadOnly);
    return frame;
}

void usage(const char* program) {
    std::fprintf(stderr,
                 "usage: %s --input recording.mov --points-width width "
                 "[--speed points_per_second] [--start seconds] [--duration seconds] "
                 "[--top-points points] [--wrap-points points] [--dump-frames]\n",
                 program);
}

}  // namespace

int main(int argc, char** argv) {
    const char* path = nullptr;
    double points_width = 0.0;
    double speed = 120.0;
    double start = 0.0;
    double duration = 0.0;
    double top_points = 0.0;
    double wrap_points = 0.0;
    bool dump_frames = false;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "--input" && i + 1 < argc) {
            path = argv[++i];
        } else if (arg == "--points-width" && i + 1 < argc) {
            points_width = std::atof(argv[++i]);
        } else if (arg == "--speed" && i + 1 < argc) {
            speed = std::atof(argv[++i]);
        } else if (arg == "--start" && i + 1 < argc) {
            start = std::atof(argv[++i]);
        } else if (arg == "--duration" && i + 1 < argc) {
            duration = std::atof(argv[++i]);
        } else if (arg == "--top-points" && i + 1 < argc) {
            top_points = std::atof(argv[++i]);
        } else if (arg == "--wrap-points" && i + 1 < argc) {
            wrap_points = std::atof(argv[++i]);
        } else if (arg == "--dump-frames") {
            dump_frames = true;
        } else {
            usage(argv[0]);
            return 2;
        }
    }
    if (!path || points_width <= 0.0 || speed <= 0.0 || start < 0.0 || duration < 0.0 ||
        top_points < 0.0 || wrap_points < 0.0) {
        usage(argv[0]);
        return 2;
    }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    NSURL* url = [NSURL fileURLWithPath:@(path)];
    AVURLAsset* asset = [AVURLAsset URLAssetWithURL:url options:nil];
    AVAssetTrack* track = [asset tracksWithMediaType:AVMediaTypeVideo].firstObject;
    if (!track) {
        std::fprintf(stderr, "animation analyzer: no video track in %s\n", path);
        return 2;
    }

    NSError* error = nil;
    AVAssetReader* reader = [[AVAssetReader alloc] initWithAsset:asset error:&error];
    NSDictionary* settings = @{
        (__bridge NSString*)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA),
    };
    AVAssetReaderTrackOutput* output = [[AVAssetReaderTrackOutput alloc] initWithTrack:track
                                                                        outputSettings:settings];
    output.alwaysCopiesSampleData = NO;
    if (![reader canAddOutput:output]) {
        std::fprintf(stderr, "animation analyzer: cannot add video decoder output\n");
        return 2;
    }
    [reader addOutput:output];
    if (![reader startReading]) {
        std::fprintf(stderr, "animation analyzer: cannot decode %s: %s\n", path,
                     reader.error.localizedDescription.UTF8String);
        return 2;
    }
#pragma clang diagnostic pop

    std::vector<FramePosition> frames;
    size_t pixel_width = 0;
    while (CMSampleBufferRef sample = [output copyNextSampleBuffer]) {
        if (CVPixelBufferRef pixels = CMSampleBufferGetImageBuffer(sample)) {
            pixel_width = CVPixelBufferGetWidth(pixels);
        }
        frames.push_back(locate_bar(sample, points_width, top_points));
        CFRelease(sample);
    }
    if (frames.size() < 3 || pixel_width == 0) {
        std::fprintf(stderr, "animation analyzer: not enough decoded frames\n");
        return 3;
    }

    const double first_time = frames.front().time;
    const double pixels_per_point = static_cast<double>(pixel_width) / points_width;
    const double speed_pixels = speed * pixels_per_point;
    std::vector<FramePosition> selected;
    for (const FramePosition& frame : frames) {
        const double time = frame.time - first_time;
        if (frame.found && time >= start && (duration == 0.0 || time <= start + duration)) {
            selected.push_back(frame);
        }
    }
    if (selected.size() < 10) {
        std::fprintf(stderr, "animation analyzer: insufficient tracked frames (%zu)\n",
                     selected.size());
        return 3;
    }

    if (wrap_points > 0.0) {
        const double wrap_pixels = wrap_points * pixels_per_point;
        double offset = 0.0;
        double previous_raw_x = selected.front().x;
        for (size_t i = 1; i < selected.size(); ++i) {
            const double raw_x = selected[i].x;
            const double delta = raw_x - previous_raw_x;
            if (delta < -wrap_pixels * 0.5) {
                offset += wrap_pixels;
            } else if (delta > wrap_pixels * 0.5) {
                offset -= wrap_pixels;
            }
            selected[i].x += offset;
            previous_raw_x = raw_x;
        }
    }

    std::vector<double> intercepts;
    for (const FramePosition& frame : selected) {
        intercepts.push_back(frame.x - speed_pixels * (frame.time - first_time));
    }
    const double intercept = quantile(intercepts, 0.5);

    std::vector<double> position_error;
    std::vector<double> step_error;
    std::vector<double> observed_speed;
    int stalled_frames = 0;
    for (size_t i = 0; i < selected.size(); ++i) {
        const FramePosition& frame = selected[i];
        const double model = intercept + speed_pixels * (frame.time - first_time);
        position_error.push_back(std::abs(frame.x - model) / pixels_per_point);
        if (i == 0) {
            continue;
        }
        const FramePosition& previous = selected[i - 1];
        const double dt = frame.time - previous.time;
        if (dt <= 0.0) {
            continue;
        }
        const double advance = frame.x - previous.x;
        const double expected_advance = speed_pixels * dt;
        step_error.push_back(std::abs(advance - expected_advance) / pixels_per_point);
        observed_speed.push_back(advance / dt / pixels_per_point);
        if (advance < expected_advance * 0.25) {
            ++stalled_frames;
        }
    }

    if (dump_frames) {
        for (const FramePosition& frame : selected) {
            const double time = frame.time - first_time;
            const double model = intercept + speed_pixels * time;
            std::printf("FRAME t=%7.3f x=%8.2f expected=%8.2f error=%7.2fpx\n", time, frame.x,
                        model, frame.x - model);
        }
    }

    std::printf("benchmark=animation frames=%zu tracked=%zu video_scale=%.3f speed=%.1fpt/s "
                "stalled=%d\n",
                frames.size(), selected.size(), pixels_per_point, speed, stalled_frames);
    print_distribution("position error", "points", position_error);
    print_distribution("step error", "points/frame", step_error);
    print_distribution("observed speed", "points/s", observed_speed);
    return 0;
}
