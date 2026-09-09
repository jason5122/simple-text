# Scroll smoothness

How trackpad scrolling reaches the glass, what was measured on the way to the current design, and
how to measure it again.

## The pipeline

`smooth_scroll` (ui/smooth_scroll.h) is the whole policy, one instance per scrolled axis; the
editor and `scroll_benchmark` both drive it the same way.

1. Precise scroll events go to `smooth_scroll::scroll` with the event's own timestamp
   (`px_event_t::timestamp`, the HID time on macOS). They never draw a frame themselves. The
   call returns true when a gesture starts, which is when the app turns the display link on and
   draws one unchanged frame.
2. While a gesture is live the window's display link runs. Each tick calls
   `smooth_scroll::tick`, which samples the input trajectory (a `scroll_predictor`) 9.5 ms behind
   the tick time and moves the offset to it; the app marks the window dirty when it changed and
   turns the display link off once `animating()` is false.
3. Core Animation's own commit at the end of the run-loop turn displays the layer, which presents
   the drawable with `presentsWithTransaction`, the same path an event-driven frame takes.

Everything else (scrollbar, keyboard, a line-based wheel) goes through `jump_to`, which ends the
gesture. The display link is started on the first event of a gesture and stopped after 20
consecutive ticks with nothing to draw, the keep-alive Chromium's Mac begin-frame source uses
(`kMaxKeepAliveCount`); restarting it costs 0.02 ms and drops no frames, so the tail only has
to outlast the 33 ms gaps at the end of a momentum tail.

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

## Resampling versus Chromium's trackpad model

Chromium resamples only touchscreens. For trackpads it shows, each frame, the sum of the deltas
that arrived since the previous frame, with a deadline that folds events arriving up to a third
of a frame after the begin-frame into the frame being built. That works because macOS momentum
events are phase-locked to the display (in a 58 s recording of real use, phase drift 0-14 us per
event, jitter 0.1-0.4 ms) and delivered with little jitter. Both models were replayed against
that recording (the accumulate mode was removed again afterwards; the user found it choppier):

| | resample | accumulate |
|---|---|---|
| frames on the 8.33 ms cadence | 4941 of 5067 | 2810 of 3936 |
| momentum step deviation p50 / p90 | 4.0% / 15.5% | 27.7% / 39.1% |
| input to glass while moving, p50 | 27.2 ms | 27.0 ms |

Accumulation depends on when events *arrive* relative to the tick; in the recording, 5 of 27
momentum gestures had events landing within 1 ms of a tick, which is where a gesture starts
alternating between empty and doubled frames. Replay delivery is jerkier than live delivery, so
the replayed accumulate numbers are pessimistic, but resampling is immune to arrival time by
construction and measured no slower, because latency is dominated by the 20 ms from tick to
glass and the event phase, not by the 9.5 ms sample offset. Without Chromium's deadline
machinery there is no room on this pipeline anyway: the tick fires about 3 ms before the vsync
whose interval the window server composites the frame in, so the commit has to leave within a
millisecond or two of it.

## Where the last 1% goes

Live, the cadence is already 99.8%: in a 58 s session of real use, 9 of 5054 moving frames landed
a refresh late, and only one of those followed a late tick. The benchmark and replays show 1-3%
because they add load. What the remaining frames are not:

- Not deadline misses on our side. Delaying the tick's hop to the main thread
  (`PX_TICK_DELAY_MS`, since removed) mapped the window server's deadline: 1 ms later and frames
  start being superseded, 2 ms later and every frame is a refresh late. The commit normally
  leaves 0.7 ms after the tick, so the margin is about 1 ms. Scheduling each frame's tick 3 ms
  ahead of the next callback (`PX_TICK_LEAD_MS`, since removed) tripled that margin and cost 2 ms
  of latency, and over 12 interleaved runs, quiet and with six busy CPU threads, dropped exactly
  as many frames (0-2 per 145) as the plain tick. The residual falls anywhere in a run.
- Not the link. CADisplayLink (macOS 14) fires 0.06 ms after a vsync where CVDisplayLink fires
  3.2 ms before it; as a drop-in every frame landed a refresh later (paint to glass 24.2 ms vs
  19.4 ms), and its full-rate request did not shorten the gesture-start ramp (30-34 ms vs 23-28
  ms from first motion to first displayed movement). It was tried and removed.

What is left is the window server's own scheduling, and a full-screen experiment confirmed it.
A game gets a perfect cadence by presenting straight to the display, which macOS only allows a
full-screen window (windowed presentation without the transaction runs at 60 Hz, above). Full
screen with the transaction is the worst case, p95 cadence a refresh late in three of three
runs; presenting straight from the command buffer with a two-drawable pool held the 8.33 ms
cadence with no dropped frames in nine of nine, at 11 ms from paint to glass when the window
server chose to scan the layer out (the Metal HUD's "Direct") and 19 or 27 ms when it
composited instead, a choice it made per run. That was tried and deliberately not kept: one
presentation path for both modes is worth more than a full-screen-only gain. ProMotion adds
variation of its own: with a 60 Hz external monitor attached the built-in panel idled at 60 Hz
for seconds and switched to 120 Hz with a visible stall, which shows up as 16.67 ms tick and
presentation intervals together.

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
- Sampling 2 ms behind the display time (an earlier design) with a half-interval extrapolation
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
out/release/scroll_benchmark /tmp/px-scroll.tsv [--input-phase-ms 4] [--fullscreen] [--dump-frames]
```

`presentation_interval` should sit at the refresh interval with a p99 no worse than one missed
refresh, `dropped_presentations` (drawables that never reached the glass) should be in the single
digits, and `presented_step_error` is the smoothness number; `presented_position_error` and
`input_to_present` are the price paid in lag. `target_to_present` and `tick_delay` separate the
window server's latency from the app's. The report tool prints the same lag for the editor as
"input to glass while moving": how long after the accumulated input reached a frame's offset
the frame appeared.

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
