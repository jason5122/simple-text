#import <AppKit/AppKit.h>
#import <ApplicationServices/ApplicationServices.h>
#import <mach/mach_time.h>

#include "experiments/platform/smoothness/scroll_trace.h"

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

#include <fcntl.h>
#include <unistd.h>

namespace {

constexpr uint64_t kSyntheticIntervalNs = 8'333'333;

using ScrollSample = scroll_trace::Sample;

class TraceWriter {
public:
    explicit TraceWriter(const char* path) {
        const int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0644);
        if (fd == -1) {
            std::fprintf(stderr, "scroll_trace: cannot create %s: %s\n", path,
                         std::strerror(errno));
            return;
        }
        output_ = fdopen(fd, "w");
        if (!output_) {
            std::fprintf(stderr, "scroll_trace: cannot open %s: %s\n", path, std::strerror(errno));
            close(fd);
            return;
        }
        setvbuf(output_, nullptr, _IOLBF, 0);
        std::fprintf(output_, "%s\n", scroll_trace::kHeader);
        std::fprintf(output_, "# time_ns\tdelta_x\tdelta_y\tscrolling_delta_x\tscrolling_delta_y"
                              "\tprecise\tphase\tmomentum_phase\tline_delta_x\tline_delta_y"
                              "\tfixed_delta_x\tfixed_delta_y\tpoint_delta_x\tpoint_delta_y"
                              "\tcontinuous\n");
    }

    ~TraceWriter() {
        if (output_) {
            std::fclose(output_);
        }
    }

    bool valid() const { return output_ != nullptr; }
    size_t sample_count() const { return sample_count_; }

    void write(const ScrollSample& sample) {
        std::fprintf(output_,
                     "%llu\t%.17g\t%.17g\t%.17g\t%.17g\t%d\t%llu\t%llu\t%lld\t%lld"
                     "\t%.17g\t%.17g\t%lld\t%lld\t%d\n",
                     static_cast<unsigned long long>(sample.time_ns), sample.delta_x,
                     sample.delta_y, sample.scrolling_delta_x, sample.scrolling_delta_y,
                     sample.precise ? 1 : 0, static_cast<unsigned long long>(sample.phase),
                     static_cast<unsigned long long>(sample.momentum_phase),
                     static_cast<long long>(sample.line_delta_x),
                     static_cast<long long>(sample.line_delta_y), sample.fixed_delta_x,
                     sample.fixed_delta_y, static_cast<long long>(sample.point_delta_x),
                     static_cast<long long>(sample.point_delta_y), sample.continuous ? 1 : 0);
        ++sample_count_;
    }

private:
    FILE* output_ = nullptr;
    size_t sample_count_ = 0;
};

ScrollSample sample_from_event(NSEvent* event, CGEventTimestamp* first_timestamp) {
    ScrollSample sample;
    sample.delta_x = event.deltaX;
    sample.delta_y = event.deltaY;
    sample.scrolling_delta_x = event.scrollingDeltaX;
    sample.scrolling_delta_y = event.scrollingDeltaY;
    sample.precise = event.hasPreciseScrollingDeltas == YES;

    CGEventRef cg_event = event.CGEvent;
    if (!cg_event) {
        return sample;
    }
    const CGEventTimestamp timestamp = CGEventGetTimestamp(cg_event);
    if (*first_timestamp == 0) {
        *first_timestamp = timestamp;
    }
    sample.time_ns = timestamp - *first_timestamp;
    sample.line_delta_x = CGEventGetIntegerValueField(cg_event, kCGScrollWheelEventDeltaAxis2);
    sample.line_delta_y = CGEventGetIntegerValueField(cg_event, kCGScrollWheelEventDeltaAxis1);
    sample.fixed_delta_x =
        CGEventGetDoubleValueField(cg_event, kCGScrollWheelEventFixedPtDeltaAxis2);
    sample.fixed_delta_y =
        CGEventGetDoubleValueField(cg_event, kCGScrollWheelEventFixedPtDeltaAxis1);
    sample.point_delta_x =
        CGEventGetIntegerValueField(cg_event, kCGScrollWheelEventPointDeltaAxis2);
    sample.point_delta_y =
        CGEventGetIntegerValueField(cg_event, kCGScrollWheelEventPointDeltaAxis1);
    sample.phase = CGEventGetIntegerValueField(cg_event, kCGScrollWheelEventScrollPhase);
    sample.momentum_phase =
        CGEventGetIntegerValueField(cg_event, kCGScrollWheelEventMomentumPhase);
    sample.continuous =
        CGEventGetIntegerValueField(cg_event, kCGScrollWheelEventIsContinuous) != 0;
    return sample;
}

}  // namespace

@interface ScrollTraceView : NSView {
    TraceWriter* _writer;
    CGEventTimestamp _firstTimestamp;
}
- (instancetype)initWithFrame:(NSRect)frame writer:(TraceWriter*)writer;
@end

@implementation ScrollTraceView

- (instancetype)initWithFrame:(NSRect)frame writer:(TraceWriter*)writer {
    self = [super initWithFrame:frame];
    if (self) {
        _writer = writer;
        _firstTimestamp = 0;
    }
    return self;
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)scrollWheel:(NSEvent*)event {
    _writer->write(sample_from_event(event, &_firstTimestamp));
}

- (void)drawRect:(NSRect)dirty_rect {
    [[NSColor windowBackgroundColor] setFill];
    NSRectFill(self.bounds);

    NSDictionary* attributes = @{
        NSFontAttributeName : [NSFont systemFontOfSize:18.0 weight:NSFontWeightMedium],
        NSForegroundColorAttributeName : [NSColor labelColor],
    };
    NSString* instructions = @"Scroll or fling over this window. Close the window when finished.";
    NSSize text_size = [instructions sizeWithAttributes:attributes];
    NSPoint origin = NSMakePoint((NSWidth(self.bounds) - text_size.width) * 0.5,
                                 (NSHeight(self.bounds) - text_size.height) * 0.5);
    [instructions drawAtPoint:origin withAttributes:attributes];
}

@end

@interface ScrollTraceDelegate : NSObject <NSApplicationDelegate> {
    TraceWriter* _writer;
    NSWindow* _window;
}
- (instancetype)initWithWriter:(TraceWriter*)writer;
@end

@implementation ScrollTraceDelegate

- (instancetype)initWithWriter:(TraceWriter*)writer {
    self = [super init];
    if (self) {
        _writer = writer;
    }
    return self;
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    const NSRect frame = NSMakeRect(0.0, 0.0, 760.0, 320.0);
    _window =
        [[NSWindow alloc] initWithContentRect:frame
                                    styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                                              NSWindowStyleMaskMiniaturizable
                                      backing:NSBackingStoreBuffered
                                        defer:NO];
    _window.title = @"Scroll Trace Recorder";
    _window.contentView = [[ScrollTraceView alloc] initWithFrame:frame writer:_writer];
    [_window center];
    [_window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
    return YES;
}

@end

namespace {

uint64_t nanoseconds_to_ticks(uint64_t nanoseconds, const mach_timebase_info_data_t& timebase) {
    const unsigned __int128 scaled = static_cast<unsigned __int128>(nanoseconds) * timebase.denom;
    return static_cast<uint64_t>(scaled / timebase.numer);
}

double ticks_to_microseconds(uint64_t ticks, const mach_timebase_info_data_t& timebase) {
    const long double nanoseconds =
        static_cast<long double>(ticks) * timebase.numer / timebase.denom;
    return static_cast<double>(nanoseconds / 1000.0L);
}

CGEventRef create_event(const ScrollSample& sample, CGEventSourceRef source, CGPoint location) {
    const auto point_y = static_cast<int32_t>(
        std::clamp<int64_t>(sample.point_delta_y, std::numeric_limits<int32_t>::min(),
                            std::numeric_limits<int32_t>::max()));
    const auto point_x = static_cast<int32_t>(
        std::clamp<int64_t>(sample.point_delta_x, std::numeric_limits<int32_t>::min(),
                            std::numeric_limits<int32_t>::max()));
    CGEventRef event =
        CGEventCreateScrollWheelEvent2(source, kCGScrollEventUnitPixel, 2, point_y, point_x, 0);
    if (!event) {
        return nullptr;
    }
    CGEventSetLocation(event, location);
    CGEventSetIntegerValueField(event, kCGScrollWheelEventDeltaAxis1, sample.line_delta_y);
    CGEventSetIntegerValueField(event, kCGScrollWheelEventDeltaAxis2, sample.line_delta_x);
    CGEventSetDoubleValueField(event, kCGScrollWheelEventFixedPtDeltaAxis1, sample.fixed_delta_y);
    CGEventSetDoubleValueField(event, kCGScrollWheelEventFixedPtDeltaAxis2, sample.fixed_delta_x);
    CGEventSetIntegerValueField(event, kCGScrollWheelEventPointDeltaAxis1, sample.point_delta_y);
    CGEventSetIntegerValueField(event, kCGScrollWheelEventPointDeltaAxis2, sample.point_delta_x);
    CGEventSetIntegerValueField(event, kCGScrollWheelEventIsContinuous, sample.continuous ? 1 : 0);
    CGEventSetIntegerValueField(event, kCGScrollWheelEventScrollPhase,
                                static_cast<int64_t>(sample.phase));
    CGEventSetIntegerValueField(event, kCGScrollWheelEventMomentumPhase,
                                static_cast<int64_t>(sample.momentum_phase));
    return event;
}

int record_trace(const char* path) {
    TraceWriter writer(path);
    if (!writer.valid()) {
        return 3;
    }

    @autoreleasepool {
        NSApplication* application = [NSApplication sharedApplication];
        application.activationPolicy = NSApplicationActivationPolicyRegular;
        ScrollTraceDelegate* delegate = [[ScrollTraceDelegate alloc] initWithWriter:&writer];
        application.delegate = delegate;
        [application run];
    }
    std::fprintf(stderr, "scroll_trace: recorded %zu samples in %s\n", writer.sample_count(),
                 path);
    return writer.sample_count() == 0 ? 4 : 0;
}

ScrollSample synthetic_sample(uint64_t time_ns,
                              double delta_y,
                              CGScrollPhase phase,
                              CGMomentumScrollPhase momentum_phase) {
    ScrollSample sample;
    sample.time_ns = time_ns;
    sample.delta_y = delta_y;
    sample.scrolling_delta_y = delta_y;
    sample.precise = true;
    sample.phase = static_cast<uint64_t>(phase);
    sample.momentum_phase = static_cast<uint64_t>(momentum_phase);
    sample.line_delta_y = static_cast<int64_t>(std::llround(delta_y));
    sample.fixed_delta_y = delta_y;
    sample.point_delta_y = static_cast<int64_t>(std::llround(delta_y));
    sample.continuous = true;
    return sample;
}

int synthesize_trace(const char* path) {
    TraceWriter writer(path);
    if (!writer.valid()) {
        return 3;
    }

    uint64_t time_ns = 0;
    writer.write(synthetic_sample(time_ns, 0.0, kCGScrollPhaseBegan, kCGMomentumScrollPhaseNone));

    constexpr double interval = 1.0 / 120.0;
    constexpr double acceleration_time = 0.10;
    constexpr double drag_time = 0.28;
    constexpr double release_velocity = 1400.0;
    for (double time = interval; time <= drag_time; time += interval) {
        const double progress = std::min(time / acceleration_time, 1.0);
        const double eased_progress = progress * progress * (3.0 - 2.0 * progress);
        time_ns += kSyntheticIntervalNs;
        writer.write(synthetic_sample(time_ns, release_velocity * eased_progress * interval,
                                      kCGScrollPhaseChanged, kCGMomentumScrollPhaseNone));
    }
    time_ns += kSyntheticIntervalNs;
    writer.write(synthetic_sample(time_ns, 0.0, kCGScrollPhaseEnded, kCGMomentumScrollPhaseNone));

    constexpr double momentum_time = 0.90;
    constexpr double decay_time = 0.22;
    bool first_momentum_sample = true;
    for (double time = 0.0; time < momentum_time; time += interval) {
        const double next_time = std::min(time + interval, momentum_time);
        const double distance = release_velocity * decay_time *
                                (std::exp(-time / decay_time) - std::exp(-next_time / decay_time));
        time_ns += kSyntheticIntervalNs;
        writer.write(synthetic_sample(time_ns, distance, static_cast<CGScrollPhase>(0),
                                      first_momentum_sample ? kCGMomentumScrollPhaseBegin
                                                            : kCGMomentumScrollPhaseContinue));
        first_momentum_sample = false;
    }
    time_ns += kSyntheticIntervalNs;
    writer.write(
        synthetic_sample(time_ns, 0.0, static_cast<CGScrollPhase>(0), kCGMomentumScrollPhaseEnd));

    std::fprintf(stderr, "scroll_trace: wrote %zu synthetic samples to %s\n",
                 writer.sample_count(), path);
    return 0;
}

int replay_trace(const char* path, pid_t pid) {
    std::vector<ScrollSample> samples;
    std::string error;
    if (!scroll_trace::read_trace(path, &samples, &error)) {
        std::fprintf(stderr, "scroll_trace: %s\n", error.c_str());
        return 3;
    }
    if (!CGPreflightPostEventAccess()) {
        std::fprintf(stderr,
                     "scroll_trace: event posting is not authorized; enable Accessibility for "
                     "this terminal in System Settings > Privacy & Security\n");
        return 5;
    }

    CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateCombinedSessionState);
    if (!source) {
        std::fprintf(stderr, "scroll_trace: could not create an event source\n");
        return 3;
    }

    mach_timebase_info_data_t timebase{};
    mach_timebase_info(&timebase);
    const uint64_t lead_time = nanoseconds_to_ticks(2'000'000'000ULL, timebase);
    const uint64_t start = mach_absolute_time() + lead_time;
    std::fprintf(stderr,
                 "scroll_trace: point at the target window; replaying %zu samples to pid %d in 2 "
                 "seconds\n",
                 samples.size(), pid);

    mach_wait_until(start);
    CGEventRef location_event = CGEventCreate(source);
    if (!location_event) {
        CFRelease(source);
        std::fprintf(stderr, "scroll_trace: could not read the pointer location\n");
        return 3;
    }
    const CGPoint location = CGEventGetLocation(location_event);
    CFRelease(location_event);

    double maximum_lateness_us = 0.0;
    for (const ScrollSample& sample : samples) {
        const uint64_t deadline = start + nanoseconds_to_ticks(sample.time_ns, timebase);
        mach_wait_until(deadline);
        const uint64_t now = mach_absolute_time();
        if (now > deadline) {
            maximum_lateness_us =
                std::max(maximum_lateness_us, ticks_to_microseconds(now - deadline, timebase));
        }
        CGEventRef event = create_event(sample, source, location);
        if (!event) {
            CFRelease(source);
            std::fprintf(stderr, "scroll_trace: could not create a scroll event\n");
            return 3;
        }
        CGEventPostToPid(pid, event);
        CFRelease(event);
    }
    CFRelease(source);
    std::fprintf(stderr, "scroll_trace: replay complete; maximum scheduler lateness %.1f us\n",
                 maximum_lateness_us);
    return 0;
}

void usage(const char* program) {
    std::fprintf(stderr,
                 "usage:\n"
                 "  %s record TRACE.tsv\n"
                 "  %s synthesize TRACE.tsv\n"
                 "  %s replay TRACE.tsv PID\n",
                 program, program, program);
}

bool parse_pid(const char* text, pid_t* pid) {
    char* end = nullptr;
    errno = 0;
    const long value = std::strtol(text, &end, 10);
    if (errno != 0 || !end || *end != '\0' || value <= 0 ||
        value > std::numeric_limits<pid_t>::max()) {
        return false;
    }
    *pid = static_cast<pid_t>(value);
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc == 3 && std::strcmp(argv[1], "record") == 0) {
        return record_trace(argv[2]);
    }
    if (argc == 3 && std::strcmp(argv[1], "synthesize") == 0) {
        return synthesize_trace(argv[2]);
    }
    if (argc == 4 && std::strcmp(argv[1], "replay") == 0) {
        pid_t pid = 0;
        if (!parse_pid(argv[3], &pid)) {
            usage(argv[0]);
            return 2;
        }
        return replay_trace(argv[2], pid);
    }
    usage(argv[0]);
    return 2;
}
