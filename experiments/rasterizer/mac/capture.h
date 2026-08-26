#pragma once

#include <cstdint>

// Window-server screen capture: take a screenshot of a window and return an image. Captures a
// window's composited backing store directly (so occlusion and window position don't matter),
// which is faster and more robust than shelling out to `screencapture` against a fixed screen
// rectangle. This is a dumb capture API -- multi-shot orchestration (the capture_server stdin
// loop and the rasterizer's --test self-capture) lives in the binaries that use it.
namespace capture {

// A crop rectangle in device pixels, top-left origin. A zero-size rect means the whole window.
struct Crop {
    int x = 0, y = 0, w = 0, h = 0;
};

// An opaque captured-and-cropped frame (a CGImageRef under the hood). Free with release_frame().
using Frame = void*;

Frame capture_frame(uint32_t window_id, Crop crop);  // null on failure
void release_frame(Frame frame);
bool frame_to_png(Frame frame, const char* out_path);

// Runs the main run loop for `seconds`, letting a window (re)composite between captures.
void pump(double seconds);

// Captures the crop once it both differs from `baseline` (the previous shot) and is stable across
// two reads -- so we never grab the still-showing previous frame nor a half-composited new one. A
// stable crop that never changes from the baseline (identical render, or a crop on empty space) is
// taken after a short grace instead of spinning. Null baseline waits on stability only. Caller
// owns the returned frame.
Frame wait_settled(uint32_t window_id, Crop crop, Frame baseline);

// CGWindowID of the frontmost on-screen window owned by a process named `owner`, or 0 if none.
uint32_t find_window(const char* owner);

// CGWindowID of the frontmost on-screen window owned by process `pid`, or 0 if none. Use this to
// target one specific Sublime Text when several are running (e.g. an editor plus an isolated test
// instance), since they share the "Sublime Text" owner name.
uint32_t find_window_for_pid(int pid);

}  // namespace capture
