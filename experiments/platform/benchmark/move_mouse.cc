// Deterministic synthetic input driver shared by both drag benchmarks.
#include <ApplicationServices/ApplicationServices.h>

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

namespace {

volatile std::sig_atomic_t interrupted = 0;

void post(CGEventType type, double x, double y) {
    CGEventRef event =
        CGEventCreateMouseEvent(nullptr, type, CGPointMake(x, y), kCGMouseButtonLeft);
    if (!event) {
        std::fprintf(stderr, "move_mouse: failed to create a mouse event\n");
        std::exit(3);
    }
    CGEventPost(kCGHIDEventTap, event);
    CFRelease(event);
}

// Keep the signal handler async-signal-safe. main() posts mouse-up before exiting, so Ctrl-C
// cannot leave the synthetic drag button stuck down.
void on_sigint(int signal) {
    interrupted = 1;
}

void usage(const char* program) {
    std::fprintf(stderr,
                 "usage: %s x0 y0 x1 y1 steps interval_us [rounds] [--drag]\n"
                 "  rounds: round trips to make (default: repeat until Ctrl-C)\n",
                 program);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 7) {
        usage(argv[0]);
        return 2;
    }

    const double x0 = std::atof(argv[1]);
    const double y0 = std::atof(argv[2]);
    const double x1 = std::atof(argv[3]);
    const double y1 = std::atof(argv[4]);
    const int steps = std::atoi(argv[5]);
    const double interval_us = std::atof(argv[6]);
    if (steps <= 0 || interval_us < 0.0) {
        usage(argv[0]);
        return 2;
    }

    long rounds = 0;
    bool drag = false;
    for (int i = 7; i < argc; ++i) {
        if (std::strcmp(argv[i], "--drag") == 0) {
            drag = true;
        } else {
            rounds = std::atol(argv[i]);
            if (rounds < 0) {
                usage(argv[0]);
                return 2;
            }
        }
    }
    const long max_legs = rounds > 0 ? rounds * 2 : 0;

    std::signal(SIGINT, on_sigint);

    double last_x = x0;
    double last_y = y0;
    post(kCGEventMouseMoved, x0, y0);
    if (drag) {
        post(kCGEventLeftMouseDown, x0, y0);
    }

    bool forward = true;
    long leg_count = 0;
    while (!interrupted && (max_legs == 0 || leg_count < max_legs)) {
        for (int i = 0; i <= steps && !interrupted; ++i) {
            double t = static_cast<double>(i) / steps;
            if (!forward) {
                t = 1.0 - t;
            }
            last_x = x0 + (x1 - x0) * t;
            last_y = y0 + (y1 - y0) * t;
            post(drag ? kCGEventLeftMouseDragged : kCGEventMouseMoved, last_x, last_y);
            usleep(static_cast<useconds_t>(interval_us));
        }
        forward = !forward;
        ++leg_count;
    }

    if (drag) {
        post(kCGEventLeftMouseUp, last_x, last_y);
    }
    return 0;
}
