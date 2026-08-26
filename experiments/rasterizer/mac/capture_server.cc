// Long-lived window-capture server. Holds ONE WindowServer connection for the whole run (per-shot
// capture processes churn connections and intermittently wedge WindowServer), and keeps the
// previous frame in memory so each shot waits for the window to actually change before grabbing
// it.
//
// Driven over stdin, one output PNG path per line: it captures the target window once it has
// changed and settled, writes the PNG, and replies "ok <path>" / "err <path>" on stdout ("quit"
// stops it). A bash loop drives Sublime Text (via `subl --command`) and feeds paths here --
// composable, and churn-free. Per-shot timing goes to stderr.
//
// Usage: capture_server (--pid <n> | --owner <name>) [--crop x,y,w,h]
#include "experiments/rasterizer/mac/capture.h"

#include <chrono>
#include <cstdio>
#include <iostream>
#include <string>
#include <string_view>

#include <spdlog/spdlog.h>

namespace {

// The stdin loop. The previous shot's frame is the change-detection baseline for the next, so a
// per-shot process never has to (and can't) share that state.
int serve(int pid, const char* owner, capture::Crop crop) {
    capture::Frame baseline = nullptr;
    std::string out_path;
    while (std::getline(std::cin, out_path)) {
        if (out_path.empty()) continue;
        if (out_path == "quit") break;
        auto t0 = std::chrono::steady_clock::now();
        uint32_t wid = pid ? capture::find_window_for_pid(pid) : capture::find_window(owner);
        capture::Frame settled = capture::wait_settled(wid, crop, baseline);
        long ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::steady_clock::now() - t0)
                      .count();
        bool ok = settled && capture::frame_to_png(settled, out_path.c_str());
        capture::release_frame(baseline);
        baseline = settled;
        std::fprintf(stderr, "%s %s (%ldms)\n", ok ? "ok" : "err", out_path.c_str(), ms);
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
        std::string_view a = argv[i];
        if (a == "--pid" && i + 1 < argc) pid = std::stoi(argv[++i]);
        else if (a == "--owner" && i + 1 < argc) owner = argv[++i];
        else if (a == "--crop" && i + 1 < argc)
            std::sscanf(argv[++i], "%d,%d,%d,%d", &crop.x, &crop.y, &crop.w, &crop.h);
    }
    if (!pid && !owner) {
        spdlog::error("usage: capture_server (--pid <n> | --owner <name>) [--crop x,y,w,h]");
        return 2;
    }
    return serve(pid, owner, crop);
}
