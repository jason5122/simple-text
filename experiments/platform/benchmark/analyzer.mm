// Analyze either benchmark's vertical cursor-to-drag-follower displacement from a screen
// recording. Symmetric up/down motion self-calibrates the fixed cursor-shape/click offset, so
// Platform's orange square and Sublime Text's dragged sidebar row produce directly comparable
// output.

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

enum class Mode { kPlatform, kSublime };

struct FramePoint {
    double time = 0.0;
    double target_y = 0.0;
    double cursor_y = 0.0;
    bool found_target = false;
    bool found_cursor = false;
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
    std::printf("%-20s n=%3zu p50=%8.3f p90=%8.3f p99=%8.3f %s\n", name, values.size(),
                quantile(values, 0.50), quantile(values, 0.90), quantile(values, 0.99), unit);
}

void locate_platform(CVPixelBufferRef pixels, FramePoint* frame) {
    CVPixelBufferLockBaseAddress(pixels, kCVPixelBufferLock_ReadOnly);
    const int width = static_cast<int>(CVPixelBufferGetWidth(pixels));
    const int height = static_cast<int>(CVPixelBufferGetHeight(pixels));
    const size_t stride = CVPixelBufferGetBytesPerRow(pixels);
    const auto* bytes = static_cast<const uint8_t*>(CVPixelBufferGetBaseAddress(pixels));

    int orange_min_x = width;
    int orange_min_y = height;
    int orange_max_x = -1;
    int orange_max_y = -1;
    // Avoid the yellow minimize button if a future capture includes the title bar.
    for (int y = height / 12; y < height; ++y) {
        const uint8_t* row = bytes + static_cast<size_t>(y) * stride;
        for (int x = 0; x < width; ++x) {
            const uint8_t b = row[static_cast<size_t>(x) * 4 + 0];
            const uint8_t g = row[static_cast<size_t>(x) * 4 + 1];
            const uint8_t r = row[static_cast<size_t>(x) * 4 + 2];
            if (r > 195 && g >= 90 && g <= 195 && b >= 15 && b <= 125) {
                orange_min_x = std::min(orange_min_x, x);
                orange_min_y = std::min(orange_min_y, y);
                orange_max_x = std::max(orange_max_x, x);
                orange_max_y = std::max(orange_max_y, y);
            }
        }
    }

    if (orange_max_x >= orange_min_x && orange_max_y >= orange_min_y) {
        frame->target_y = (orange_min_y + orange_max_y + 1) * 0.5;
        frame->found_target = true;

        constexpr int kHorizontalSearchRadius = 80;
        const int center_x = (orange_min_x + orange_max_x + 1) / 2;
        const int x0 = std::max(0, center_x - kHorizontalSearchRadius);
        const int x1 = std::min(width, center_x + kHorizontalSearchRadius + 1);
        double white_y = 0.0;
        int white_count = 0;
        for (int y = 0; y < height; ++y) {
            const uint8_t* row = bytes + static_cast<size_t>(y) * stride;
            for (int x = x0; x < x1; ++x) {
                const uint8_t b = row[static_cast<size_t>(x) * 4 + 0];
                const uint8_t g = row[static_cast<size_t>(x) * 4 + 1];
                const uint8_t r = row[static_cast<size_t>(x) * 4 + 2];
                if (r > 225 && g > 225 && b > 225 &&
                    std::max({r, g, b}) - std::min({r, g, b}) < 12) {
                    white_y += y + 0.5;
                    ++white_count;
                }
            }
        }
        if (white_count >= 6 && white_count <= 200) {
            frame->cursor_y = white_y / white_count;
            frame->found_cursor = true;
        }
    }

    CVPixelBufferUnlockBaseAddress(pixels, kCVPixelBufferLock_ReadOnly);
}

void locate_sublime(CVPixelBufferRef pixels, FramePoint* frame) {
    CVPixelBufferLockBaseAddress(pixels, kCVPixelBufferLock_ReadOnly);
    const int width = static_cast<int>(CVPixelBufferGetWidth(pixels));
    const int height = static_cast<int>(CVPixelBufferGetHeight(pixels));
    const size_t stride = CVPixelBufferGetBytesPerRow(pixels);
    const auto* bytes = static_cast<const uint8_t*>(CVPixelBufferGetBaseAddress(pixels));

    // The capture is 420 points wide and the synthetic cursor stays at x=80 points. At the usual
    // Retina scale its white interior stays in this narrow strip.
    double white_y = 0.0;
    int white_count = 0;
    for (int y = 70; y < height - 20; ++y) {
        const uint8_t* row = bytes + static_cast<size_t>(y) * stride;
        for (int x = 150; x < std::min(width, 195); ++x) {
            const uint8_t b = row[static_cast<size_t>(x) * 4 + 0];
            const uint8_t g = row[static_cast<size_t>(x) * 4 + 1];
            const uint8_t r = row[static_cast<size_t>(x) * 4 + 2];
            if (r > 247 && g > 247 && b > 247 && std::max({r, g, b}) - std::min({r, g, b}) < 8) {
                white_y += y + 0.5;
                ++white_count;
            }
        }
    }
    if (white_count >= 6 && white_count <= 160) {
        frame->cursor_y = white_y / white_count;
        frame->found_cursor = true;
    }

    // Average columns containing neither label text/cursor nor the disclosure marker. The dragged
    // selection band is the only dark feature spanning both column ranges below the sidebar
    // header.
    const int sidebar_right = std::min(width, 500);
    std::vector<double> level(static_cast<size_t>(height), 0.0);
    for (int y = 80; y < height - 20; ++y) {
        const uint8_t* row = bytes + static_cast<size_t>(y) * stride;
        double sum = 0.0;
        int count = 0;
        for (int x = 4; x < std::min(sidebar_right, 26); ++x) {
            sum += row[static_cast<size_t>(x) * 4 + 0] + row[static_cast<size_t>(x) * 4 + 1] +
                   row[static_cast<size_t>(x) * 4 + 2];
            count += 3;
        }
        for (int x = 250; x < std::min(sidebar_right, 430); ++x) {
            sum += row[static_cast<size_t>(x) * 4 + 0] + row[static_cast<size_t>(x) * 4 + 1] +
                   row[static_cast<size_t>(x) * 4 + 2];
            count += 3;
        }
        level[static_cast<size_t>(y)] = count ? sum / count : 255.0;
    }

    std::vector<double> background_samples;
    for (int y = 150; y < height - 40; y += 4) {
        background_samples.push_back(level[static_cast<size_t>(y)]);
    }
    const double threshold = quantile(background_samples, 0.65) - 3.0;

    int best_start = -1;
    int best_end = -1;
    double best_level = 1e9;
    for (int y = 80; y < height - 20;) {
        if (level[static_cast<size_t>(y)] >= threshold) {
            ++y;
            continue;
        }
        const int start = y;
        double sum = 0.0;
        while (y < height - 20 && level[static_cast<size_t>(y)] < threshold) {
            sum += level[static_cast<size_t>(y++)];
        }
        const int end = y;
        if (end - start >= 14) {
            const double mean = sum / (end - start);
            if (mean < best_level) {
                best_level = mean;
                best_start = start;
                best_end = end;
            }
        }
    }
    if (best_start >= 0) {
        frame->target_y = (best_start + best_end) * 0.5;
        frame->found_target = true;
    }

    CVPixelBufferUnlockBaseAddress(pixels, kCVPixelBufferLock_ReadOnly);
}

void usage(const char* program) {
    std::fprintf(stderr,
                 "usage: %s --mode platform|sublime --input recording.mov "
                 "--points-width width [--dump-frames]\n",
                 program);
}

}  // namespace

int main(int argc, char** argv) {
    Mode mode = Mode::kPlatform;
    const char* path = nullptr;
    double points_width = 0.0;
    bool dump_frames = false;
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "--mode" && i + 1 < argc) {
            const char* value = argv[++i];
            if (std::strcmp(value, "platform") == 0) {
                mode = Mode::kPlatform;
            } else if (std::strcmp(value, "sublime") == 0) {
                mode = Mode::kSublime;
            } else {
                usage(argv[0]);
                return 2;
            }
        } else if (arg == "--input" && i + 1 < argc) {
            path = argv[++i];
        } else if (arg == "--points-width" && i + 1 < argc) {
            points_width = std::atof(argv[++i]);
        } else if (arg == "--dump-frames") {
            dump_frames = true;
        } else {
            usage(argv[0]);
            return 2;
        }
    }
    if (!path || points_width <= 0.0) {
        usage(argv[0]);
        return 2;
    }

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    NSURL* url = [NSURL fileURLWithPath:@(path)];
    AVURLAsset* asset = [AVURLAsset URLAssetWithURL:url options:nil];
    AVAssetTrack* track = [asset tracksWithMediaType:AVMediaTypeVideo].firstObject;
    if (!track) {
        std::fprintf(stderr, "benchmark analyzer: no video track in %s\n", path);
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
        std::fprintf(stderr, "benchmark analyzer: cannot add video decoder output\n");
        return 2;
    }
    [reader addOutput:output];
    if (![reader startReading]) {
        std::fprintf(stderr, "benchmark analyzer: cannot decode %s: %s\n", path,
                     reader.error.localizedDescription.UTF8String);
        return 2;
    }
#pragma clang diagnostic pop

    std::vector<FramePoint> frames;
    size_t pixel_width = 0;
    while (CMSampleBufferRef sample = [output copyNextSampleBuffer]) {
        CVPixelBufferRef pixels = CMSampleBufferGetImageBuffer(sample);
        if (pixels) {
            FramePoint frame;
            frame.time = CMTimeGetSeconds(CMSampleBufferGetPresentationTimeStamp(sample));
            if (mode == Mode::kPlatform) {
                locate_platform(pixels, &frame);
            } else {
                locate_sublime(pixels, &frame);
            }
            pixel_width = CVPixelBufferGetWidth(pixels);
            frames.push_back(frame);
        }
        CFRelease(sample);
    }

    if (frames.size() < 3 || pixel_width == 0) {
        std::fprintf(stderr, "benchmark analyzer: not enough decoded frames\n");
        return 3;
    }
    const double pixels_per_point = static_cast<double>(pixel_width) / points_width;
    if (dump_frames) {
        for (size_t i = 0; i < frames.size(); ++i) {
            const FramePoint& frame = frames[i];
            std::printf("FRAME %3zu t=%7.3f target=%d y=%8.2f cursor=%d y=%8.2f offset=%8.2f\n", i,
                        frame.time, frame.found_target ? 1 : 0, frame.target_y,
                        frame.found_cursor ? 1 : 0, frame.cursor_y,
                        frame.cursor_y - frame.target_y);
        }
    }

    struct MovingSample {
        double offset;
        double velocity;
    };
    std::vector<MovingSample> moving;
    std::vector<double> down_offsets;
    std::vector<double> up_offsets;
    for (size_t i = 1; i + 1 < frames.size(); ++i) {
        const FramePoint& previous = frames[i - 1];
        const FramePoint& frame = frames[i];
        const FramePoint& next = frames[i + 1];
        if (!previous.found_cursor || !frame.found_cursor || !next.found_cursor ||
            !frame.found_target) {
            continue;
        }
        const double before_dt = frame.time - previous.time;
        const double after_dt = next.time - frame.time;
        if (before_dt <= 0.0 || after_dt <= 0.0) {
            continue;
        }
        const double before_velocity = (frame.cursor_y - previous.cursor_y) / before_dt;
        const double after_velocity = (next.cursor_y - frame.cursor_y) / after_dt;
        if (before_velocity * after_velocity <= 0.0) {
            continue;
        }
        const double velocity = (before_velocity + after_velocity) * 0.5;
        if (std::abs(velocity) / pixels_per_point < 1000.0) {
            continue;
        }
        const double offset = frame.cursor_y - frame.target_y;
        moving.push_back(MovingSample{offset, velocity});
        (velocity > 0.0 ? down_offsets : up_offsets).push_back(offset);
    }

    if (down_offsets.empty() || up_offsets.empty()) {
        std::fprintf(
            stderr,
            "benchmark analyzer: insufficient constant-direction frames (down=%zu up=%zu)\n",
            down_offsets.size(), up_offsets.size());
        return 3;
    }
    const double down_median = quantile(down_offsets, 0.5);
    const double up_median = quantile(up_offsets, 0.5);
    const double calibrated_offset = (down_median + up_median) * 0.5;

    std::vector<double> cursor_lead;
    std::vector<double> along_path_lead;
    std::vector<double> inferred_lag;
    std::vector<double> sampled_speed;
    for (const MovingSample& sample : moving) {
        const double lead_pixels =
            (sample.offset - calibrated_offset) * std::copysign(1.0, sample.velocity);
        const double lead_points = lead_pixels / pixels_per_point;
        cursor_lead.push_back(std::abs(lead_points));
        along_path_lead.push_back(lead_points);
        inferred_lag.push_back(lead_pixels / std::abs(sample.velocity) * 1000.0);
        sampled_speed.push_back(std::abs(sample.velocity) / pixels_per_point);
    }

    const int targets = static_cast<int>(std::count_if(
        frames.begin(), frames.end(), [](const FramePoint& frame) { return frame.found_target; }));
    const int cursors = static_cast<int>(std::count_if(
        frames.begin(), frames.end(), [](const FramePoint& frame) { return frame.found_cursor; }));
    std::printf("benchmark=%s frames=%zu target=%d cursor=%d moving=%zu video_scale=%.3f "
                "calibrated_offset=%.2fpx down=%.2fpx up=%.2fpx\n",
                mode == Mode::kPlatform ? "platform" : "sublime", frames.size(), targets, cursors,
                moving.size(), pixels_per_point, calibrated_offset, down_median, up_median);
    print_distribution("cursor lead", "points", cursor_lead);
    print_distribution("along-path lead", "points", along_path_lead);
    print_distribution("inferred lag", "ms", inferred_lag);
    print_distribution("sampled speed", "points/s", sampled_speed);
    return 0;
}
