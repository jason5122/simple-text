#include <ApplicationServices/ApplicationServices.h>

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <unistd.h>

namespace {

volatile std::sig_atomic_t interrupted = 0;

void move_pointer(double x, double y) {
    CGEventRef event =
        CGEventCreateMouseEvent(nullptr, kCGEventMouseMoved, CGPointMake(x, y), kCGMouseButtonLeft);
    if (!event) {
        std::fprintf(stderr, "scroll_wheel: failed to create a mouse event\n");
        std::exit(3);
    }
    CGEventPost(kCGHIDEventTap, event);
    CFRelease(event);
}

void click_pointer(double x, double y) {
    for (CGEventType type : {kCGEventLeftMouseDown, kCGEventLeftMouseUp}) {
        CGEventRef event =
            CGEventCreateMouseEvent(nullptr, type, CGPointMake(x, y), kCGMouseButtonLeft);
        if (!event) {
            std::fprintf(stderr, "scroll_wheel: failed to create a mouse event\n");
            std::exit(3);
        }
        CGEventPost(kCGHIDEventTap, event);
        CFRelease(event);
    }
}

void post_scroll(int delta) {
    CGEventRef event =
        CGEventCreateScrollWheelEvent(nullptr, kCGScrollEventUnitPixel, 1, delta);
    if (!event) {
        std::fprintf(stderr, "scroll_wheel: failed to create a scroll event\n");
        std::exit(3);
    }
    CGEventSetIntegerValueField(event, kCGScrollWheelEventIsContinuous, 1);
    CGEventPost(kCGHIDEventTap, event);
    CFRelease(event);
}

void on_sigint(int signal) {
    (void)signal;
    interrupted = 1;
}

void usage(const char* program) {
    std::fprintf(stderr,
                 "usage: %s x y delta steps interval_us [rounds]\n"
                 "  rounds: alternating up/down round trips (default: repeat until Ctrl-C)\n",
                 program);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 6 || argc > 7) {
        usage(argv[0]);
        return 2;
    }

    const double x = std::atof(argv[1]);
    const double y = std::atof(argv[2]);
    const int delta = std::atoi(argv[3]);
    const int steps = std::atoi(argv[4]);
    const double interval_us = std::atof(argv[5]);
    const long rounds = argc == 7 ? std::atol(argv[6]) : 0;
    if (delta == 0 || steps <= 0 || interval_us < 0.0 || rounds < 0) {
        usage(argv[0]);
        return 2;
    }

    std::signal(SIGINT, on_sigint);
    move_pointer(x, y);
    click_pointer(x, y);
    usleep(100'000);

    const long max_legs = rounds > 0 ? rounds * 2 : 0;
    long leg = 0;
    while (!interrupted && (max_legs == 0 || leg < max_legs)) {
        const int signed_delta = leg % 2 == 0 ? delta : -delta;
        for (int i = 0; i < steps && !interrupted; ++i) {
            post_scroll(signed_delta);
            usleep(static_cast<useconds_t>(interval_us));
        }
        ++leg;
    }
    return 0;
}
