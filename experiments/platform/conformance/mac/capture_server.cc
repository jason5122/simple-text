// Long-lived window-capture server. Holds one WindowServer connection for the whole run and keeps
// the previous frame in memory so each shot waits for the window to actually change before it is
// captured.
//
// Driven over stdin, one output PNG path per line: it captures the target window once it has
// changed and settled, writes the PNG, and replies "ok <path>" / "err <path>" on stdout ("quit"
// stops it). Per-shot timing goes to stderr.
//
// Usage: capture_server (--pid <n> | --owner <name>) [--crop x,y,w,h]
#include "experiments/platform/conformance/capture.h"
#include <chrono>
#include <cstdio>
#include <iostream>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>

namespace {

int serve(int pid, const char* owner, capture::Crop crop) {
    capture::Frame baseline = nullptr;
    std::string out_path;
    while (std::getline(std::cin, out_path)) {
        if (out_path.empty()) continue;
        if (out_path == "quit") break;
        auto start = std::chrono::steady_clock::now();
        capture::WindowId window_id =
            pid ? capture::find_window_for_pid(pid) : capture::find_window(owner);
        capture::Frame settled = capture::wait_settled(window_id, crop, baseline);
        long milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::steady_clock::now() - start)
                                .count();
        bool ok = settled && capture::frame_to_png(settled, out_path.c_str());
        capture::release_frame(baseline);
        baseline = settled;
        if (ok) {
            std::fprintf(stderr, "%s (%ldms)\n", out_path.c_str(), milliseconds);
        } else {
            std::fprintf(stderr, "err %s (%ldms)\n", out_path.c_str(), milliseconds);
        }
        std::printf("%s %s\n", ok ? "ok" : "err", out_path.c_str());
        std::fflush(stdout);
    }
    capture::release_frame(baseline);
    return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    std::setbuf(stdout, nullptr);
    capture::Crop crop;
    int pid = 0;
    const char* owner = nullptr;
    for (int i = 1; i < argc; i++) {
        std::string_view argument = argv[i];
        if (argument == "--pid" && i + 1 < argc) {
            pid = std::stoi(argv[++i]);
        } else if (argument == "--owner" && i + 1 < argc) {
            owner = argv[++i];
        } else if (argument == "--crop" && i + 1 < argc) {
            std::sscanf(argv[++i], "%d,%d,%d,%d", &crop.x, &crop.y, &crop.w, &crop.h);
        }
    }
    if (!pid && !owner) {
        spdlog::error("usage: capture_server (--pid <n> | --owner <name>) [--crop x,y,w,h]");
        return 2;
    }
    return serve(pid, owner, crop);
}
