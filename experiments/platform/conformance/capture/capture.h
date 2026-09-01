#pragma once

#include <cstdint>

// Window-server screen capture: take a screenshot of a window and return an image. Captures a
// window's composited backing store directly (so occlusion and window position don't matter),
// which is faster and more robust than shelling out to `screencapture` against a fixed screen
// rectangle. This is a dumb capture API -- multi-shot orchestration lives in the binary using it.
namespace capture {

using WindowId = uintptr_t;

// A crop rectangle in device pixels, top-left origin. A zero-size rect means the whole window.
struct Crop {
    int x = 0, y = 0, w = 0, h = 0;
};

// An opaque captured-and-cropped frame. Free with release_frame().
using Frame = void*;

Frame capture_frame(WindowId window_id, Crop crop);  // null on failure
void release_frame(Frame frame);
bool frame_to_png(Frame frame, const char* out_path);

// Runs the main run loop for `seconds`, letting a window (re)composite between captures.
void pump(double seconds);

// Captures the crop once it both differs from `baseline` (the previous shot) and is stable across
// two reads. Null baseline waits on stability only. Caller owns the returned frame.
Frame wait_settled(WindowId window_id, Crop crop, Frame baseline);

// Native id of the frontmost on-screen window owned by a process named `owner`, or 0 if none.
WindowId find_window(const char* owner);

// Native id of the frontmost on-screen window owned by process `pid`, or 0 if none.
WindowId find_window_for_pid(int pid);

}  // namespace capture
