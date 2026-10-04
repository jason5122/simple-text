// Drives any application's window with a recorded trackpad scroll, posted at the HID tap so the
// window server routes it exactly as it would a real trackpad. This is how Sublime Text, Chromium
// and our own editor get the same gesture under the same observer (Instruments' Display
// instruments, see benchmark/README.md "Measuring other apps"). The in-process replay in
// scroll_benchmark stays the tool for our own pipeline; this one exists only for comparisons
// across applications, which nothing in-process can do.
//
//   drive_scroll --pid PID [--lead-ms 2000] TRACE.tsv
//
// The target application is activated and the pointer warped to the centre of PID's largest
// on-screen window first, since a scroll goes to whatever is under the pointer. Accepts both trace formats: px-scroll-trace-v1 (with scroll
// phases, from the removed HID recorder) and px-scroll-trace-v2 (time, dx, dy, precise, from
// scroll_benchmark --record), the latter tagged as one drag: Began, Changed for every sample, Ended.
// Needs Accessibility permission for the terminal.

#import <AppKit/AppKit.h>
#include <ApplicationServices/ApplicationServices.h>
#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <mach/mach_time.h>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

namespace {

struct Sample {
    uint64_t time_ns = 0;
    double delta_x = 0.0;
    double delta_y = 0.0;
    bool precise = true;
    uint64_t phase = 0;
    uint64_t momentum_phase = 0;
    int64_t line_delta_x = 0;
    int64_t line_delta_y = 0;
    double fixed_delta_x = 0.0;
    double fixed_delta_y = 0.0;
    int64_t point_delta_x = 0;
    int64_t point_delta_y = 0;
    bool continuous = true;
};

bool parse_v1(const std::string& line, Sample* s) {
    double scrolling_x = 0.0;
    double scrolling_y = 0.0;
    int precise = 0;
    int continuous = 0;
    std::istringstream in(line);
    if (!(in >> s->time_ns >> s->delta_x >> s->delta_y >> scrolling_x >> scrolling_y >> precise >>
          s->phase >> s->momentum_phase >> s->line_delta_x >> s->line_delta_y >> s->fixed_delta_x >>
          s->fixed_delta_y >> s->point_delta_x >> s->point_delta_y >> continuous)) {
        return false;
    }
    s->precise = precise != 0;
    s->continuous = continuous != 0;
    return true;
}

bool parse_v2(const std::string& line, Sample* s) {
    int precise = 0;
    std::istringstream in(line);
    if (!(in >> s->time_ns >> s->delta_x >> s->delta_y >> precise)) {
        return false;
    }
    s->precise = precise != 0;
    s->continuous = s->precise;
    s->phase = kCGScrollPhaseChanged;
    s->fixed_delta_x = s->delta_x;
    s->fixed_delta_y = s->delta_y;
    s->point_delta_x = static_cast<int64_t>(std::llround(s->delta_x));
    s->point_delta_y = static_cast<int64_t>(std::llround(s->delta_y));
    s->line_delta_x = static_cast<int64_t>(std::llround(s->delta_x / 10.0));
    s->line_delta_y = static_cast<int64_t>(std::llround(s->delta_y / 10.0));
    return true;
}

bool read_trace(const char* path, std::vector<Sample>* samples) {
    std::ifstream in(path);
    if (!in) {
        std::fprintf(stderr, "drive_scroll: cannot open %s\n", path);
        return false;
    }
    int version = 0;
    std::string line;
    while (std::getline(in, line)) {
        if (line == "# px-scroll-trace-v1") {
            version = 1;
            continue;
        }
        if (line == "# px-scroll-trace-v2") {
            version = 2;
            continue;
        }
        if (line.empty() || line[0] == '#') {
            continue;
        }
        Sample s;
        const bool ok = version == 1 ? parse_v1(line, &s) : version == 2 ? parse_v2(line, &s) : false;
        if (!ok) {
            std::fprintf(stderr, "drive_scroll: cannot parse %s (version %d): %s\n", path, version,
                         line.c_str());
            return false;
        }
        samples->push_back(s);
    }
    if (samples->empty()) {
        std::fprintf(stderr, "drive_scroll: %s has no samples\n", path);
        return false;
    }
    if (version == 2) {
        // One drag: a phase-Began event at rest before the first sample, Ended after the last.
        Sample began = samples->front();
        began.delta_x = began.delta_y = began.fixed_delta_x = began.fixed_delta_y = 0.0;
        began.point_delta_x = began.point_delta_y = began.line_delta_x = began.line_delta_y = 0;
        began.phase = kCGScrollPhaseBegan;
        Sample ended = began;
        ended.time_ns = samples->back().time_ns + 1'000'000;
        ended.phase = kCGScrollPhaseEnded;
        samples->insert(samples->begin(), began);
        samples->push_back(ended);
    }
    return true;
}

CGEventRef create_event(const Sample& s, CGEventSourceRef source, CGPoint location) {
    const auto clamp32 = [](int64_t v) {
        return static_cast<int32_t>(std::clamp<int64_t>(v, std::numeric_limits<int32_t>::min(),
                                                        std::numeric_limits<int32_t>::max()));
    };
    CGEventRef event = CGEventCreateScrollWheelEvent2(source, kCGScrollEventUnitPixel, 2,
                                                      clamp32(s.point_delta_y),
                                                      clamp32(s.point_delta_x), 0);
    if (!event) {
        return nullptr;
    }
    CGEventSetLocation(event, location);
    CGEventSetIntegerValueField(event, kCGScrollWheelEventDeltaAxis1, s.line_delta_y);
    CGEventSetIntegerValueField(event, kCGScrollWheelEventDeltaAxis2, s.line_delta_x);
    CGEventSetDoubleValueField(event, kCGScrollWheelEventFixedPtDeltaAxis1, s.fixed_delta_y);
    CGEventSetDoubleValueField(event, kCGScrollWheelEventFixedPtDeltaAxis2, s.fixed_delta_x);
    CGEventSetIntegerValueField(event, kCGScrollWheelEventPointDeltaAxis1, s.point_delta_y);
    CGEventSetIntegerValueField(event, kCGScrollWheelEventPointDeltaAxis2, s.point_delta_x);
    CGEventSetIntegerValueField(event, kCGScrollWheelEventIsContinuous, s.continuous ? 1 : 0);
    CGEventSetIntegerValueField(event, kCGScrollWheelEventScrollPhase,
                                static_cast<int64_t>(s.phase));
    CGEventSetIntegerValueField(event, kCGScrollWheelEventMomentumPhase,
                                static_cast<int64_t>(s.momentum_phase));
    return event;
}

// The centre of the pid's largest on-screen window, in global (top-left) coordinates.
bool window_centre(pid_t pid, CGPoint* centre) {
    CFArrayRef list = CGWindowListCopyWindowInfo(
        kCGWindowListOptionOnScreenOnly | kCGWindowListExcludeDesktopElements, kCGNullWindowID);
    if (!list) {
        return false;
    }
    double best_area = 0.0;
    for (CFIndex i = 0; i < CFArrayGetCount(list); ++i) {
        CFDictionaryRef info = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(list, i));
        int owner = 0;
        int layer = 0;
        CFNumberRef owner_ref = static_cast<CFNumberRef>(CFDictionaryGetValue(info, kCGWindowOwnerPID));
        CFNumberRef layer_ref = static_cast<CFNumberRef>(CFDictionaryGetValue(info, kCGWindowLayer));
        if (owner_ref) {
            CFNumberGetValue(owner_ref, kCFNumberIntType, &owner);
        }
        if (layer_ref) {
            CFNumberGetValue(layer_ref, kCFNumberIntType, &layer);
        }
        if (owner != pid || layer != 0) {
            continue;
        }
        CGRect bounds = CGRectZero;
        CFDictionaryRef bounds_ref =
            static_cast<CFDictionaryRef>(CFDictionaryGetValue(info, kCGWindowBounds));
        if (!bounds_ref || !CGRectMakeWithDictionaryRepresentation(bounds_ref, &bounds)) {
            continue;
        }
        const double area = bounds.size.width * bounds.size.height;
        if (area > best_area) {
            best_area = area;
            *centre = CGPointMake(CGRectGetMidX(bounds), CGRectGetMidY(bounds));
        }
    }
    CFRelease(list);
    return best_area > 0.0;
}

uint64_t ns_to_ticks(uint64_t ns, const mach_timebase_info_data_t& tb) {
    return static_cast<uint64_t>(static_cast<unsigned __int128>(ns) * tb.denom / tb.numer);
}

uint64_t ticks_to_ns(uint64_t ticks, const mach_timebase_info_data_t& tb) {
    return static_cast<uint64_t>(static_cast<unsigned __int128>(ticks) * tb.numer / tb.denom);
}

}  // namespace

int main(int argc, char** argv) {
    pid_t pid = 0;
    double lead_ms = 2000.0;
    const char* trace_path = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--pid") == 0 && i + 1 < argc) {
            pid = static_cast<pid_t>(std::atoi(argv[++i]));
        } else if (std::strcmp(argv[i], "--lead-ms") == 0 && i + 1 < argc) {
            lead_ms = std::atof(argv[++i]);
        } else {
            trace_path = argv[i];
        }
    }
    if (!pid || !trace_path) {
        std::fprintf(stderr, "usage: drive_scroll --pid PID [--lead-ms MS] TRACE.tsv\n");
        return 2;
    }
    std::vector<Sample> samples;
    if (!read_trace(trace_path, &samples)) {
        return 3;
    }
    if (!CGPreflightPostEventAccess()) {
        std::fprintf(stderr, "drive_scroll: not allowed to post events; grant Accessibility to "
                             "this terminal in System Settings > Privacy & Security\n");
        return 5;
    }
    CGPoint centre;
    if (!window_centre(pid, &centre)) {
        std::fprintf(stderr, "drive_scroll: pid %d has no on-screen window\n", pid);
        return 4;
    }
    CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateCombinedSessionState);
    if (!source) {
        return 3;
    }
    // Bring the target to the front, so no other window sits under the pointer.
    if (NSRunningApplication* app =
            [NSRunningApplication runningApplicationWithProcessIdentifier:pid]) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        [app activateWithOptions:NSApplicationActivateIgnoringOtherApps];
#pragma clang diagnostic pop
        usleep(300'000);
    }
    // Warp, then move, so the window server's hover state follows the pointer.
    CGWarpMouseCursorPosition(centre);
    if (CGEventRef move = CGEventCreateMouseEvent(source, kCGEventMouseMoved, centre,
                                                  kCGMouseButtonLeft)) {
        CGEventPost(kCGHIDEventTap, move);
        CFRelease(move);
    }

    mach_timebase_info_data_t tb{};
    mach_timebase_info(&tb);
    const uint64_t start =
        mach_absolute_time() + ns_to_ticks(static_cast<uint64_t>(lead_ms * 1e6), tb);
    std::fprintf(stderr, "drive_scroll: pointer at %.0f,%.0f; %zu samples start in %.0f ms\n",
                 centre.x, centre.y, samples.size(), lead_ms);
    // A long wait wakes late (timer coalescing); wake early, then wait precisely.
    mach_wait_until(start - ns_to_ticks(100'000'000ULL, tb));
    mach_wait_until(start);

    double worst_late_us = 0.0;
    for (const Sample& s : samples) {
        const uint64_t deadline = start + ns_to_ticks(s.time_ns, tb);
        mach_wait_until(deadline);
        const uint64_t now = mach_absolute_time();
        if (now > deadline) {
            worst_late_us = std::max(worst_late_us, ticks_to_ns(now - deadline, tb) / 1000.0);
        }
        CGEventRef event = create_event(s, source, centre);
        if (!event) {
            CFRelease(source);
            return 3;
        }
        // The scheduled time, not the posting time: a late wake-up then looks like a late
        // delivery of a regular device, which is what trackpad HID timestamps give an app.
        CGEventSetTimestamp(event, ticks_to_ns(deadline, tb));
        CGEventPost(kCGHIDEventTap, event);
        CFRelease(event);
    }
    CFRelease(source);
    std::printf("drive_scroll: done; start_mach_s=%.6f worst_lateness_us=%.1f\n",
                ticks_to_ns(start, tb) / 1e9, worst_late_us);
    return 0;
}
