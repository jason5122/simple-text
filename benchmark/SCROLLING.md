# Scroll smoothness

How trackpad scrolling reaches the glass, what was measured on the way to the current design, and
how to measure it again.

## The pipeline

1. Scroll events feed a `scroll_predictor` (ui/scroll_predictor.h) with the event's own timestamp
   (`px_event_t::timestamp`, the HID time on macOS). They never draw a frame themselves.
2. While a gesture is live the window's display link runs. Each tick samples the input trajectory
   9.5 ms behind the tick time, sets the scroll offset to that, and marks the window dirty.
3. Core Animation's own commit at the end of the run-loop turn displays the layer, which presents
   the drawable with `presentsWithTransaction`, the same path an event-driven frame takes.

The sample point is one 120 Hz input interval plus delivery jitter behind the tick, so it normally
falls inside the recent events and is interpolated. A tick whose interval received no event (the
input and display clocks always drift against each other) still advances along the line instead
of repeating a frame and then doubling up. The predictor extrapolates at most one input interval.
Phase-only events (delta zero) are not fed to it. Chromium's LinearResampling samples 5 ms behind
its vsync-aligned frame time for the same reason; it only resamples touchscreens, so its trackpad
smoothness comes from the vsync-paced frame production, which is what step 2 reproduces.

The predictor is not Chromium's two-point interpolation, because a real trackpad stream (captured
2026-09-09 from the built-in trackpad) is not a clean trajectory at the event scale:

- A finger drag arrives at 240 Hz as pairs of events with uneven deltas (3 then 21 points for
  24 points of motion), sometimes bunched 1.3 ms apart.
- Momentum events arrive at 120 Hz carrying the delta for a nominal step, but individual
  timestamps land up to 3 ms late (an 11.3 ms gap followed by a 5.3 ms one) while the deltas stay
  on the decay curve. The rate then drops to 60 Hz and 30 Hz as the motion dies.

So `scroll_predictor` first pulls a late timestamp back toward the stream's cadence (the median of
the last five intervals, by at most 4 ms so a real pause is kept), then fits a line through the
events of the last 24 ms and evaluates it at the sample time. Replaying that captured flick into
the editor, the per-frame steps through the momentum phase went from 22, 16, 32, 37, 7, 33, 21,
39 with two-point interpolation to 25, 31, 36, 35, 41, 45, 40, 37 with the fit.

## What was measured (2026-09-09, M4 Max, built-in ProMotion panel, windowed)

- Displaying the layer from the tick (`displayIfNeeded`, with or without an explicit
  `CATransaction`) made the window server discard 70-90% of the presented drawables, with stalls
  of up to 900 ms. Leaving the display to Core Animation's commit gives a full 120 Hz cadence.
- Presenting drawables directly (`presentsWithTransaction = NO`) never dropped a frame but ran
  the windowed layer at 60 Hz with 40 ms paint-to-present. That is the 54 fps sawtooth the Metal
  HUD showed. Full screen bypasses the window server, which is why it looked fine there.
- Transaction presentation costs a fixed 19 ms from paint to `presentedTime`: the tick fires
  11.6 ms before the display link's output time and the frame lands one refresh after it. Roughly
  3-5% of frames land a refresh late; `nextDrawable` never blocks and the tick's delivery to the
  main thread varies by under 0.1 ms, so the misses are on the window server's side.
- Sampling 2 ms behind the display time (the previous design) with a half-interval extrapolation
  cap left the sampled value equal to "latest input plus a constant", so a tick without a new
  event repeated the previous frame: 29 doubled intervals in 68 during a replayed gesture in the
  editor. Sampling behind the tick removed all of them.
- The first `CVDisplayLink` a process creates takes ~30 ms. px now creates it in the background
  when the window is made and only starts and stops it.
- The window server leaves its idle refresh rate on the first commit, not on the first touch. The
  editor commits an unchanged frame when a gesture begins (macOS sends the touch before the
  motion), which cut the time from first motion to first displayed movement from 78 ms to 25 ms.
- When the finger stops dead, the trajectory extrapolates up to a frame of motion past the
  finger's final position. The editor used to snap back to the accumulated input 50 ms later and
  then re-apply the extrapolated sample: a −29 pt frame followed by a +29 pt frame, recorded and
  replayed from a real gesture. It now rests where the trajectory ended, and a gesture that
  resumes after a pause (2.5 cadences, at least 25 ms) restarts its trajectory from the content's
  position rather than stalling until the input catches up.

## Measuring

Build the tools and a deterministic 120 Hz trace:

```sh
bin/ninja -C out/release scroll_benchmark scroll_trace editor
out/release/scroll_trace synthesize /tmp/px-scroll.tsv
```

`scroll_benchmark` plays the trace from a mach-clock thread with a chosen phase against the
display link and reports `presentedTime` feedback:

```sh
benchmark/run_scroll_comparison.sh out/release /tmp/px-scroll.tsv
out/release/scroll_benchmark /tmp/px-scroll.tsv --mode resampled --sample-offset-ms 9.5 --dump-frames
```

`presentation_interval` should sit at the refresh interval with a p99 no worse than one missed
refresh, `dropped_presentations` (drawables that never reached the glass) should be in the single
digits, and `presented_step_error` is the smoothness number; `presented_position_error` and
`input_to_present` are the price paid in lag. `target_to_present` and `tick_delay` separate the
window server's latency from the app's.

## Recording your own scrolling

```sh
benchmark/record_editor_scroll.sh flick        # scroll; click in the text at a bad frame; Cmd+Q
benchmark/replay_editor_scroll.sh /tmp/editor-scroll-flick.tsv
```

The first opens the editor with `EDITOR_SCROLL_TRACE=1`, which logs one line per scroll event
(its timestamp, the interval since the previous one, and how late the main thread saw it), per
display tick, per presented frame with the step it moved, and per click (`mark`). When the
editor quits, `benchmark/scroll_log_report.py` turns the log into a px-scroll-trace-v1 file with
the marks as `# mark` comments and prints a report: for each mark, every frame of the 1.5 s
before it, with the frames that would look wrong flagged (never displayed by the window server,
landed a refresh late, a step out of line with its neighbours) and any input that reached the
main thread late. Without marks it lists every flagged frame.

The second replays the trace at the HID tap into a fresh editor (keep your hands off the
trackpad; the pointer is warped over the window) and prints the same report around the same
marks, so a recording's glitch can be reproduced and a fix judged on it. Replayed events are
stamped with their recorded timestamps, so the app sees the device's timing rather than the
replayer's. Phases are not recorded, and the editor clamps at offset zero, so start recordings
from the top of the document and scroll down.

A smooth run shows `interval 8.333 ms` on every frame during the gesture and steps that change
gradually; a repeated step of 0 followed by a double step is the beat pattern.

macOS changes window compositing and ProMotion cadence with visibility and system load. Check
`top -o cpu` for WindowServer and browsers before trusting a run, and run each configuration
several times.
