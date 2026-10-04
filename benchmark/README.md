# Benchmark

macOS only: the numbers come from the Metal backend's presentation telemetry.

## Scroll paths

The editor has two scroll and drag paths, chosen at compile time from the SDK the binary is
linked against (`PX_OS_SMOOTHS_EVENT_FRAMES` in px/px.h; `vtool -show-build BIN` shows the SDK,
which is what AppKit reads). Linked against the macOS 26 SDK or later, AppKit displays the layer
once per refresh at a vsync-locked point, so the editor paints each scroll and drag event in its
own turn with no display link (Sublime Text's model). Linked against an older SDK (`out/release`,
whose `mac_sdk_path` is the vendored 14 SDK), AppKit displays at the event's own phase and the
window server discards paired commits, so the display link paints, sampling the input a little
behind each tick (ui/smooth_scroll). `out/release-sdk26` builds against the system SDK. The
benchmarks and `scroll_demo` default to the build's path and take `--per-event` and `--tick` (or
`--policy event|tick`) to measure the other.

## Regression check

```bash
benchmark/check.sh
```

Replays `benchmark/traces/flick.tsv` (a 4.2 s trackpad flick, 518 events) and
`benchmark/traces/drag.tsv` (a 4.2 s thumb drag, 464 events) through the two benchmarks below on
the editor's own scroll and drag paths, and prints the lines that decide whether something
regressed. Each replay opens a window for about five seconds; keep hands off the trackpad. Run it
two or three times and read the best run: the window server shares the display with everything
else, and a run with `missed_display_ticks` in the dozens or a `presentation_interval` p95 above
one refresh was disturbed, not regressed. Runs are comparable only within one power state; the
script prints it first, since the server runs at half rate in Low Power Mode and every number
changes.

What the flick read on 2026-10-01 in high power, on this machine (M4 Max, 120 Hz panel), under
the `predict` policy of that day (since removed; baselines for the two paths that remain are
still to be taken, on AC power): `presentation_interval` p50 and p95 at 8.333 ms,
`input_to_present` p50 38-39 ms, `presented_step_error` p50 4-6 pt, `dropped_presentations` in
the single digits with about 55 `superseded_presentations` (the trace carries 240 Hz event
pairs, and the second paint of a pair replaces the first; see benchmark/SCROLLING.md,
"Latency"), `distance_ratio` 1.000. The drag: `presentation_interval` p50 and p95 at 8.333 ms,
`input_to_present` p50 31-33 ms, `step_deviation` p50 2-4 pt, `never_displayed` and
`late_frames` in the single digits.

## Scrolling

```bash
out/release/scroll_benchmark --record /tmp/flick.tsv            # scroll by hand; Escape ends it
out/release/scroll_benchmark --replay /tmp/flick.tsv > /tmp/flick-replay.txt
```

Both modes print the same report. `--record` measures the live gesture, events arriving through
AppKit, and `--replay` the in-process timer, and on the 26-SDK window the two can differ
(benchmark/SCROLLING.md, "Live events on the 26-SDK window"), so a change is judged on both.
`--stop-after-idle 3` ends a recording three seconds after the last event, which lets
`drive_scroll --pid PID TRACE.tsv` post a recorded trace at the HID tap into the recording window
for a live replay without a hand. `run_loop_busy` and `run_loop_sleep` are the main thread's
wake-to-sleep and sleep-to-wake segments while the gesture ran: a busy p95 above a refresh is the
app blocking, a sleep p95 near a refresh with `sample_delivery_lag` to match is input waiting in
the queue.

## Scrollbar Dragging

```bash
out/release/scrollbar_benchmark --record /tmp/drag.tsv
out/release/scrollbar_benchmark --replay /tmp/drag.tsv > /tmp/drag-replay.txt
```

## Resizing

Any px program can log how it resizes. Set `PX_RESIZE_TRACE=1`, drag the window's corner, quit,
and read the summary from stderr:

```bash
PX_RESIZE_TRACE=1 out/release/editor 2> /tmp/editor-resize.txt
PX_RESIZE_TRACE=1 out/release/dark_mode_demo 2> /tmp/demo-resize.txt
```

Each resize step should get exactly one frame. `folded` steps were never painted because the
next step arrived first; `extra_frames` are paints no step asked for. `paint_gap` over 1.5
refreshes and `late` presentations are the hitches, and `never_displayed` frames were painted but
never reached the glass. `step_gap` is the input's own pacing, so `paint_gap` and `late` counts that
match it are the hand pausing, not a stall; `step_to_paint` and `step_to_present` are the latency
from a step to its frame, whatever the pacing. `acquire` is the wait for a drawable, which grows with window size, so compare
programs at the same size. Metal only, and Core Animation reports no presented times for the
frames of a live drag, so during a drag the presentation lines are empty and the cadence is read
from `paint_gap` against `step_gap`. Check WindowServer in `top -o cpu` first: under load even a
perfect renderer folds steps.

## Measuring other apps

Two tools compare any application against ours on the glass, with the same gesture and the same
observer, whatever the application draws with:

```bash
# Sublime Text (the conformance copy), a long non-repetitive file open at line 8000
conformance/sublime_text_4212.app/Contents/SharedSupport/bin/subl /tmp/long_source.txt:8000
out/release/glass_capture --pid $(pgrep -f sublime_text_4212.app/Contents/MacOS/sublime_text) \
    --seconds 12 --display --inset-top 40 --inset-bottom 30 --out /tmp/st.tsv &
sleep 2; out/release/drive_scroll --pid <same pid> --lead-ms 500 /tmp/editor-scroll-flick.tsv

# the editor reduced to scrolling, same file, same line, a chosen scroll policy
out/release-sdk26/scroll_demo /tmp/long_source.txt --line 8000 --policy event &
```

`drive_scroll` activates the target, warps the pointer onto its window and posts a recorded
trackpad gesture at the HID tap (Accessibility permission). `glass_capture` records what the
display shows of the window through ScreenCaptureKit (Screen Recording permission): per frame,
how far the text moved, found by hashing each pixel row and letting rows vote for a shift, so the
content must not repeat line for line. Use `--display` (vsync-quantized frame times; the window
mode's are jittery) and insets that exclude tab and status bars, whose rows never move. The
capture runs at the panel's rate: 120 fps in high power, 60 in Low Power Mode. The analyzer that
turns a capture into cadence, step regularity, held frames, travel against the input and
input-to-glass lag is still a scratch script (/tmp/glass_report.py); if the rig stays, it becomes
a tool next to the benchmarks.
