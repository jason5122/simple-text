// Long-lived native Linux capture server for the Sublime reference suite.
//
// Wayland: uses Mutter's private RecordWindow D-Bus API and consumes the resulting PipeWire
// stream. RecordWindow without a window-id attaches to the focused window, so focus Sublime before
// starting the server. This captures the MetaWindow directly, not a rectangle of the desktop.
//
// X11/XWayland: uses XCompositeNameWindowPixmap to read the redirected backing pixmap, independent
// of stacking and occlusion.

#include "experiments/platform/conformance/linux/capture_frame.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace {

using linux_capture::Crop;
using linux_capture::Frame;
using linux_capture::Source;

constexpr auto kFrameTimeout = std::chrono::seconds(3);
constexpr auto kFrameQuiet = std::chrono::milliseconds(100);
constexpr auto kFirstFrameGrace = std::chrono::milliseconds(200);
constexpr auto kPoll = std::chrono::milliseconds(10);

// A window that is being driven produces a frame every time; several in a row that produce nothing
// means the stream is attached to something that never repaints, and every remaining shot in the
// run would burn kFrameTimeout to write nothing useful.
constexpr int kMaxConsecutiveTimeouts = 2;

bool wait_settled(Source* source, Crop crop, const Frame* baseline, Frame* result) {
    const auto started = std::chrono::steady_clock::now();
    auto last_change = started;
    bool saw_change = false;
    Frame latest;

    while (std::chrono::steady_clock::now() - started < kFrameTimeout) {
        Frame current;
        if (source->next_frame(&current, crop, kPoll)) {
            const bool changed_from_last = latest.rgba.empty() || current != latest;
            latest = std::move(current);
            if (changed_from_last) last_change = std::chrono::steady_clock::now();
            if (!baseline || latest != *baseline) saw_change = true;
        }

        const auto now = std::chrono::steady_clock::now();
        if (!latest.rgba.empty() && saw_change && now - last_change >= kFrameQuiet) {
            *result = std::move(latest);
            return true;
        }
        if (!latest.rgba.empty() && !saw_change && now - started >= kFirstFrameGrace) {
            *result = std::move(latest);
            return true;
        }
    }
    if (!latest.rgba.empty()) {
        *result = std::move(latest);
        return true;
    }
    return false;
}

int serve(std::unique_ptr<Source> source, Crop crop) {
    Frame baseline;
    bool have_baseline = false;
    int consecutive_timeouts = 0;
    std::string output_path;
    while (std::getline(std::cin, output_path)) {
        if (output_path.empty()) continue;
        if (output_path == "quit") break;
        const auto started = std::chrono::steady_clock::now();
        Frame settled;
        const bool captured =
            wait_settled(source.get(), crop, have_baseline ? &baseline : nullptr, &settled);
        const bool ok = captured && linux_capture::write_png(settled, output_path.c_str());
        const long milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                                      std::chrono::steady_clock::now() - started)
                                      .count();
        std::fprintf(stderr, "%s%s (%ldms)\n", ok ? "" : "err ", output_path.c_str(),
                     milliseconds);
        std::printf("%s %s\n", ok ? "ok" : "err", output_path.c_str());
        std::fflush(stdout);
        if (captured) {
            consecutive_timeouts = 0;
            baseline = std::move(settled);
            have_baseline = true;
            continue;
        }
        if (++consecutive_timeouts >= kMaxConsecutiveTimeouts) {
            std::fprintf(stderr,
                         "no frames for %d captures in a row: the stream is attached to a window "
                         "that is not repainting. Mutter records whichever window had focus when "
                         "this server started, so anything that took focus is being captured "
                         "instead of Sublime Text.\n",
                         consecutive_timeouts);
            return 1;
        }
    }
    return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    std::setbuf(stdout, nullptr);
    Crop crop;
    int logical_top_inset = 0;
    int pid = 0;
    const char* owner = nullptr;
    std::string backend = "auto";
    for (int i = 1; i < argc; ++i) {
        const std::string_view argument = argv[i];
        if (argument == "--pid" && i + 1 < argc) {
            pid = std::stoi(argv[++i]);
        } else if (argument == "--owner" && i + 1 < argc) {
            owner = argv[++i];
        } else if (argument == "--crop" && i + 1 < argc) {
            std::sscanf(argv[++i], "%d,%d,%d,%d", &crop.x, &crop.y, &crop.width, &crop.height);
        } else if (argument == "--logical-top-inset" && i + 1 < argc) {
            logical_top_inset = std::stoi(argv[++i]);
        } else if (argument == "--backend" && i + 1 < argc) {
            backend = argv[++i];
        }
    }
    if ((!pid && !owner) || (backend != "auto" && backend != "mutter" && backend != "x11")) {
        std::fprintf(stderr, "usage: capture_server (--pid <n> | --owner <name>) "
                             "[--backend auto|mutter|x11] [--crop x,y,w,h]\n");
        return 2;
    }

    std::string error;
    std::unique_ptr<Source> source;
    bool includes_server_decorations = false;
    const char* wayland_display = std::getenv("WAYLAND_DISPLAY");
    if (backend != "x11" && wayland_display && *wayland_display) {
        source = linux_capture::make_mutter_source(&error);
        includes_server_decorations = source != nullptr;
        if (!source && backend == "auto") {
            std::fprintf(stderr, "Mutter capture unavailable: %s; trying X11\n", error.c_str());
        }
    }
    const char* x_display = std::getenv("DISPLAY");
    if (!source && backend != "mutter" && x_display && *x_display) {
        source = linux_capture::make_x11_source(pid, owner, &error);
    }
    if (!source) {
        std::fprintf(stderr, "could not initialize Linux window capture: %s\n", error.c_str());
        return 1;
    }
    // Mutter records the compositor's MetaWindow, including its server-side title bar. The X11
    // backend names the client's redirected pixmap, which already excludes a reparenting WM's
    // frame, so applying the Wayland inset there would discard application content.
    if (logical_top_inset != 0 && includes_server_decorations) {
        const int device_inset =
            static_cast<int>(std::lround(logical_top_inset * source->device_scale()));
        crop.y += device_inset;
        std::fprintf(stderr, "logical top inset %d at %.3gx scale: %d device pixels\n",
                     logical_top_inset, source->device_scale(), device_inset);
    }
    return serve(std::move(source), crop);
}
