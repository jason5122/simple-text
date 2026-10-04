# Scroll smoothness

How trackpad scrolling reaches the glass, what was measured on the way to the current design, and
how to measure it again.

## The pipeline

Two paths, chosen at compile time from the SDK the binary is linked against
(`PX_OS_SMOOTHS_EVENT_FRAMES` in px/px.h, from `__MAC_OS_X_VERSION_MAX_ALLOWED`; `vtool
-show-build` shows the SDK AppKit reads). Linked against the macOS 26 SDK or later, AppKit's
window displays the layer once per refresh at a vsync-locked point, so the editor paints each
scroll and drag event's frame in the event's own turn, with no display link and nothing sampled:
Sublime Text's model (ST 4212 is linked against the 26.5 SDK). Linked against an older SDK, AppKit
displays at the event's own phase and the window server discards paired commits, so the display
link paints from the input's trajectory as described below. Decided 2026-10-02; the frame pacer
and the scroll modes of 2026-09-30 to 10-02 were removed the same day (see "Pacing to the window
server"), leaving the tick path as it was on 2026-09-29 plus `smooth_scroll::track` for drags.

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

## What was measured (2026-09-30, Low Power Mode, same machine)

Low Power Mode is a deterministic reproduction of the half-rate state first seen in the wild on
2026-09-29: the window server shows the layer every 16.67 ms while the panel's display link keeps
ticking every 8.33 ms. Read the two intervals together; a tick interval that stays at 8.33 ms
while the presentation interval doubles is this state, and both at 16.67 ms is the panel idling
at 60 Hz, which needs nothing.

- With the CVDisplayLink tick, every tick paints, so two drawables are committed per shown frame
  and the window server discards one: 449 of 912 never displayed, presentation p50 = p95 =
  16.67 ms, shown frames queued three refreshes behind their tick target (25 ms, 8 ms quiet),
  input to glass 38-41 ms (26 quiet). Which frame of the pair shows flips now and then, which is
  where the large step errors come from.
- A `CADisplayLink` does not follow this state: probes bound to a bare screen and to a presenting
  window both report 8.333 ms intervals and `duration`, with and without a preferred 120 Hz range.
- `PX_CA_DISPLAY_LINK=1` (the window's `CADisplayLink` as the tick source, opt in, macOS 14+)
  still ticks at 120 Hz, but with a display link on the run loop Core Animation holds the layer's
  display over to the next cycle, where it absorbs the following tick too: 470 paints for 977
  ticks, 2 never displayed, 459 of 467 presentation intervals exactly two refreshes. Step
  regularity (|step − mean of its neighbours|) improved from p95 45 / max 204 pt to p95 33 /
  max 94 pt; latency did not (paint to glass 32.7 vs 32.0 ms, input to glass 43 vs 38-41 ms).
  `PX_CA_DISPLAY_LINK=vote` (preferredFrameRateRange at the panel maximum) measured the same.
- The same fold happens with Low Power Mode off, which rules the link out as a drop-in: on the
  120 Hz panel the `CADisplayLink` tick paints and presents every 16.67 ms (470 paints, 0-4 never
  displayed, 446-455 of ~468 intervals at two refreshes) where the `CVDisplayLink` tick gives
  898 of 915 intervals at one refresh. Input to glass was 28.8 ms against 31-32 for CV, because
  each shown frame carries the later of two ticks' input. Drawing inside the callback
  (`displayIfNeeded` after the tick) to defeat the fold left 32 of 912 frames on the glass with
  gaps up to 940 ms, the same failure as displaying from the CV tick in 2026-09-09's notes. The
  mode was removed; `=1` and `=vote` remain for measurement.
- The scrollbar drag replay shows the same state: presentation 16.67 ms, 379 of 827 never
  displayed; its `refresh=` summary field is derived from presentation and reads 16.667.

## Pacing to the window server (2026-09-30)

**Removed 2026-10-02.** The pacer, its perf log and tools (`px_frame_pacing`, `px_perf_log`,
`perf_report`, `pacer_replay`), the tick hold (`kTickDelayAfterVsyncSeconds`), painting from the
tick's turn (`px_display_now`) and the scroll modes other than resample (`resample_keep`,
`accumulate`, `accumulate_predict`, `kBiasDecay`) went well beyond what Chromium does, and the
compile-time split in "The pipeline" leaves the half-rate case to the window server on the
per-event path. What follows is the record of what was built and measured.

`px_frame_pacer` (px/mac/px_frame_pacing.h) fixes the half-rate case without leaving
`CVDisplayLink`. It attributes each paint to the tick that caused it and reads the presented time
Core Animation reports per drawable. Twelve tick frames with four or more discards and shown
frames two refreshes apart put it in half-rate mode: only every second tick is delivered, at the
phase whose frames the server had been showing. Measured on the Low Power Mode flick against the
unpaced run: 7 of 460 paints discarded instead of 449 of 912 (the remainder are the gesture-start
event paints replacing a tick frame of the same content, which the benchmark now reports as
`unchanged_presentations`), 96% of shown intervals exactly two refreshes, and nothing changes in
the normal state (no mode change in 1065 ticks, 95% of intervals one refresh, under load). Latency
is the server's: shown frames land three refreshes after their tick target in Low Power Mode with
or without pacing.

Leaving half-rate mode is the hard part, and the log of the first attempts is why the design is
what it is:

- A probe tick next to a kept tick collides with it on one side or the other, and which of the two
  the server keeps depends on where its latch falls: the first probe design discarded the kept
  frame (a hitch) every two seconds, and a probe placed before the kept tick did the same half the
  time. Two commits in one half-rate slot are also sometimes both stamped with the same presented
  time, which fooled the exit test once. So probes happen only in the first two ticks after
  animation restarts, where the content is at rest and a discard is invisible, with one fallback
  probe after ten seconds of continuous animation; a pair counts as full rate only if shown
  clearly a refresh apart.
- Exiting on a drop in the kept frames' latency (3 refreshes paced in Low Power Mode, 2 on the
  normal server at the same cadence) misfired twice: the first frames after a mode change carry
  the previous mode's backlog, and the first frames after a pause in the input are shown faster.
  Removed.
- Admitting a make-up tick after a kept tick that painted nothing never recovered the lost
  refresh: its frame always shared the next kept frame's latch window. Removed.
- Delaying paced commits by 3.5 ms to land after the vsync, where the `CADisplayLink` path had
  committed, made frames miss the latch instead (latency p95 25 to 42 ms). The early commit is the
  right one.
- What remains and is not ours: a kept tick that finds no motion loses its refresh when input
  restarts right after it (the same odds as the `CADisplayLink` fold), and a few frames per run
  are shown a refresh early, a one-then-three pattern the server produces on its own.

`PX_FRAME_PACER=0` disables the pacer for comparison.

### Resize frames are not evidence (2026-10-01)

The editor's first day of field logs showed the pacer locking to a half, a third and a quarter of
the rate on a server showing every refresh, six times in four runs, each during or within 150 ms
of a live resize and once 1.1 s after one. A resize paints a frame per step at the resize's own
pace, Core Animation reports no presented time for most of them and irregular ones for the rest,
and twelve such frames looked enough like half rate to pass the test. That was the scrolling that
turned laggy after a resize, and the rubber band at the ends of the document came with it: paced
ticks run `accumulate_predict`'s prediction, and a prediction at the clamped end of the range
stepped past the edge and was taken back a frame later (offsets as far as 155 points past the top
in the logs, and never while the pacer was at full rate, where the frames come from the events).
Found in the same logs: a gesture's first paced tick, before the stream's cadence is known, showed
nothing and left a bias that the next frames worked off, so paced gestures started a step behind.

Now frames painted during a resize and for 300 ms after it are not evidence, the shown intervals
of a candidate window must sit at whole multiples of the candidate divisor, predictions are
clamped like everything else, and a tick with a new event shows it whatever the predictor knows.
`out/release/pacer_replay LOG.tsv` replays a log through the pacer as built and prints its mode
changes beside the recorded ones: on the four resize logs it makes none of the six wrong changes,
and on the three Low Power Mode logs it reproduces every genuine one within 90 ms.

## macOS 15.5 (2026-10-01, M2 Max, built-in ProMotion panel, high power)

The first data from another OS: three 20-30 s hand sessions of the editor with the same binary,
each a few scroll gestures and some thumb dragging, read with `perf_report` and by feel.

| run | one-refresh intervals | irregular steps | input to glass p50 | felt |
|---|---|---|---|---|
| default (predict, painted on the event) | 82% | 3.5% | 23.0 ms | choppiest, rarely smooth |
| `SMOOTH_SCROLL_MODE=resample` | 99% | 3.3% | 24.6 ms | flawless |
| `SMOOTH_SCROLL_MODE=accumulate` | 96% | 5.2% | 22.3 ms | smooth with hiccups |

The first row is not a measurement of the event-painted path. At 1.0 s, half a second into the
run's first gesture, the server discarded every other frame for six pairs running with the
shown frames exactly two refreshes apart, the half-rate signature (read at the time as a genuine
dip; "The tick's phase" below shows it was frames on the server's deadline colliding, the
survivors shown a refresh early); the pacer paced on it at 1.1 s
and, since the gesture never paused and the pacer only probes at a restart or after ten seconds,
stayed paced until a restart probe at 9.4 s. Every scroll gesture of that run fell between 0.5
and 8.8 s. So what felt choppy was scrolling at 60 fps with four events per frame on a server
that had almost certainly recovered (nothing like that dip recurs in 80 s of logs across the
three runs; what does recur, in all of them, is short discard-shown-discard hiccups that the
pacer rightly ignores). The event-painted path has about half a second of unpaced scrolling in
these logs, too little to judge; `PX_FRAME_PACER=0` is the way to feel it on 15.5 on its own.
Dragging, which is tick-painted on every path, felt smooth in all three runs.

What the other two rows do say: resample is flawless on 15.5 by feel and 99% by the log, and
accumulate's hiccups show in the log as the largest frame-to-frame step changes (p90 43% of the
step, against 26% for resample), the phase effect of summing on the tick. The 15.5 trackpad
delivers uneven 240 Hz pairs (+40 then +110 points, 4 ms apart) for 10-20% of its intervals,
which the tick paths absorb by summing. The 15.5 server also presents a refresh sooner than 26.5
(paint to glass 19.8 ms against 24.4), so the paths sit within 2 ms of each other on latency.

The pacer's evidence rule cannot tell a 100 ms dip from Low Power Mode; only an earlier probe
could, and a probe mid-gesture at genuine half rate costs a visible hitch (see above). One probe
a second after pacing begins would bound a transient's cost to a second at the price of one
possible hitch per paced gesture in Low Power Mode. That is a trade-off to decide, not a bug like
the resize one.

A fourth run, `PX_FRAME_PACER=0`, one 17 s gesture, felt "very close to resample, 97% flawless".
At the time that switch only stopped the tick halving; the editor still read the pacer's divisor
to choose event painting, and the pacer detected half rate again at 3.0 s, so the run is
event-painted for 0-3 s and 13-17 s and tick-painted predict in between (fixed since: the switch
now also reports a divisor of 1). Split that way the log is unambiguous:

| segment | shown frames | a refresh later than usual | two-refresh holds per second | paint pairs |
|---|---|---|---|---|
| event-painted, 0-3 s | 274 | 28% | 3.0 | 30 |
| tick-painted predict, 3-13 s | 1156 | 0% | 1.0 | 11 |
| event-painted, 13-17 s | 444 | 47% | 2.1 | 62 |

So on 15.5 a frame committed at the event's phase misses the server's latch a quarter to a half
of the time, where a tick's frame, committed 3 ms before the vsync, never does; the misses are
not consistent, so they show as holds, three a second against one. The second half-rate
detection also sits right after a burst of double paints with discards (2.82-2.87 s, 240 Hz
pairs), as the first did, and neither tick-painted run saw one: the working hypothesis is that
two commits per refresh make the 15.5 server drop to half rate for a moment, which the pacer then
reads as its state. Both effects come from painting at the event's phase, and both are absent on
26.5, where the server queues a frame deeper (two refreshes of latency instead of one) and
absorbs the phase. The event-painted path is a 26.5 win on latency and a 15.5 loss on smoothness;
the tick-painted paths hold on both.

## The resample latency: 9.5, 7 and 5 ms (2026-10-01, M4 Max, on battery at full rate)

Chromium's LinearResampling samples 5 ms behind its frame time; ours samples 9.5 ms behind the
tick. Tried both and 7 ms on the checked-in flick and drag traces, two replays each, with the
per-event drag path (Sublime's model, as AppKit on the 26 SDK paces it) as the reference for
how tight a thumb can follow the pointer. Numbers are run-to-run noisy at this sample size; the
drag's irregular-step count varied 19-35 within one setting.

| | drag: position error p50 | drag: irregular steps | drag: input to present | flick: position error p50 | flick: step error p50 |
|---|---|---|---|---|---|
| 9.5 ms | 152-178 pt | 19-35 | 27-32 ms | 235-244 pt | 5.2-5.4 pt |
| 7 ms | 120-123 pt | 34-63 | 24 ms | 221-222 pt | 4.7-4.9 pt |
| 5 ms | 136-142 pt | 39-45 | 26-27 ms | 209-210 pt | 4.7-4.8 pt |
| per-event drag | 134-138 pt | 33-34 | 28-29 ms | | |

Position error is the content's distance from where the pointer's (or finger's) trajectory says
it should be at the moment the frame shows: the puck lag, in content points (11 per point of
pointer travel on this drag). Shortening the latency tightens the thumb by about 2-3 points of
pointer travel, to where the per-event path sits, and costs irregular steps, as the unit test
`TrackKeepsMovingThroughATickThatSawNoNewTarget` says it must: a drag's sample is clamped to
the pointer's last position, so the latency is the only headroom a tick has when its event is
late; at 5 ms the content sits a quarter-step from the pointer, a late event leaves the tick
almost nothing to move, and the event then lands as a double step. The flick is not clamped
that way (the sample may run up to 8 ms past the newest event) and shows no smoothness cost
across the three settings in the replay, but the replay delivers events 1.7 ms after their
timestamps where AppKit takes several, so in the editor a 5 ms sample sits past the newest event
more often than the replay shows; the field log is the judge there.

The per-event drag is no faster in time than the tick path on the 26 SDK, since AppKit defers its
display to a vsync-locked point, and no smoother. Kept at 9.5 ms: the decision of the day was
smoothness over latency, and the thumb's extra 2-3 points are the same trade.

Where Chromium's `frame_time - 5 ms` lands in our pipeline depends on which vsync plays
`frame_time`. From the logs: the tick fires 3.2 ms before a vsync V0, AppKit paints the tick's
frame 0.6 ms after V0, and the server shows it at V3. Chromium decides a frame's content in the
interval that begins at `frame_time`, after waiting up to a third of it for late input; we decide
at the tick, in the interval that begins at V-1. Aligned by decision interval, `frame_time - 5 ms`
is V-1 - 5 ms = 10.1 ms behind the tick, within 0.6 ms of where we are, and the 5 and 7 ms runs
above were already past Chromium. Aligned instead by the interval the frame is painted in,
`frame_time` is V0 and the sample sits 1.8 ms behind the tick. That reading was tried as well,
with Chromium's cap of half an input interval on extrapolation in place of our full interval:
flick position error fell to 170-186 points with step error unchanged in the replay, but three
resample unit tests fail under it, the telling one being `KeepsMovingThroughATickThatSawNoNewEvent`
(a tick whose event is late bridges 4.3 points of a 12-point step instead of 12), and the replay
delivers events 1.7 ms after their timestamps where AppKit takes several, so in the editor that
tick is the common case, not the rare one. Chromium can afford the half-interval cap because it
waits for the late event; we would need the wait to go with it. Not adopted.

## The tick's phase (2026-10-01)

A hand session on the resample default read laggy, and its log showed the pacer at half rate
for seven of the ten seconds of scrolling: detections at 34.1 s and 40.1 s, each on a clean run
of alternating discards with the shown frames two refreshes apart. The server was not at half
rate. In both windows the surviving frames were shown a refresh *earlier* than the session's
usual latency (1.0 against 2.0), where the genuine half rate of the Low Power Mode logs shows
them a refresh *later* (3.0). The frames were sitting on the window server's deadline.

The model that fits every log: a commit before a deadline about 3 ms before a vsync is shown two
refreshes after that vsync on 26.5, a commit after it one refresh later still, and the deadline
wanders by about a millisecond from frame to frame. A frame painted near it lands on either side
at random; a slow frame followed by a fast one is committed into the same slot and discarded,
and the result is the half-rate signature. Where the tick's frame was painted, from the logs:

| session | paint after the tick | position in the refresh | result |
|---|---|---|---|
| 2026-09-30 editor, resample | 3.8 ms | 0.6 ms after the vsync | latency 2.0 always, no pacer |
| 2026-10-01 05:35 benchmarks, resample | 0.1 ms | 3.1 ms before the vsync | latency 1.0, 40% of frames at 2.0 |
| 2026-10-01 05:43 editor, resample | 1.3 ms | 1.9 ms before the vsync | collisions, two false detections |
| 2026-10-01 editor on 15.5, event-painted | at the event | 0.6 ms after the vsync | 28-47% a refresh late, two false detections |

The display link's callback comes 3.2 ms before the vsync and has nothing to do with when the
frame is committed; where the paint lands has depended on how much work sits between the tick
and Core Animation's commit, which is incidental and changed without anyone changing it. The
15.5 rows say the deadline there sits about 0.6 ms after the vsync rather than 3 ms before it,
which is what the event-painted path kept hitting, and why its "half-rate dips" and the pacer
lock-ins that followed were these collisions too, not the server.

Fixed by choosing the phase: the callback now holds the tick on the link's thread until 1 ms
after the vsync it precedes (`kTickDelayAfterVsyncSeconds`; the main thread picks it up about
0.8 ms later), so the editor paints about 3 ms after the vsync and the benchmarks about 2. The
window between deadlines that both systems share, from the model above, runs from about 1.6 to
4.3 ms after the vsync; a 2 ms hold was tried first and put the editor's paint at 4.1, on its
edge. Consequences to verify: the latency is the same every frame (2.0 on 26.5, as on Sep 30);
the scroll sample is 5 ms fresher, since the tick now follows the scroll event AppKit delivers
right after the vsync instead of preceding it; and 15.5 may show one refresh more latency than
its lucky phase gave it, in exchange for never sitting on the deadline.

The load that moved the paint was a game (Slippi Dolphin) running alongside, WindowServer at 45%
of a core. Under it, Core Animation commits in one of two modes, in stretches: right away at the
end of the run-loop turn (the paint 0.1 ms after the tick in the benchmarks, 1.3 ms in the
editor) or deferred to the next vsync (the paint a constant 5.3 ms after the tick, landing 0.1 to
1.2 ms after a vsync). Before the fix the immediate mode landed on the deadline; after it both
modes land inside the window, and a run under the same load reads flat: two editor sessions at
94% and 97% one-refresh intervals, latency 3.0 at p50 and p95 with no pacer line, against the
pre-fix session's SERVER_LATE verdict, 15% of frames a refresh late and a false detection at
9.5 s. The price under load is the deferred mode's refresh: input to glass 29 ms in the editor
and 31.7 ms on the flick replay, against 27 ms unloaded on the new phase and 30 before it. The
replays under load otherwise match the quiet ones (0 to 3 frames dropped, every interval at one
refresh at p95).

It came back the same evening. A ten-minute session paced to half rate at 592.9 s on the same
collision signature, survivors at 2 refreshes against the session's 3. The tick sat where the
hold put it, 1.6 ms after the vsync; the paint followed it by 4.3 ms this time, landing 2.4 ms
before the next vsync, on the deadline. The distance from tick to paint is Core Animation's
commit timing, and it differs by session with nothing changed in the code: 1.3 ms in the morning
session, 3.8 on Sep 30, 5.4 under the game, 4.3 that evening, each tight within its session. The
benchmarks paint 0.1 ms after the tick in every session because their frame goes out at the end
of the tick's own run-loop turn. Pinning the tick pinned the wrong end.

Two changes, both verified against every log of the day with `pacer_replay` and the pacer's unit
rig. The tick's frame is now painted in the tick's own turn (`px_display_now` from the tick, as
the event path did), so it follows the tick by a fixed fraction of a millisecond, and the hold is
1.5 ms, putting the commit about 2.5 ms after the vsync. And the pacer no longer takes the
signature at face value: half rate shows the survivors a refresh later than full rate did, so the
pacer remembers the latency of the most recent clean full-rate stretch (six or more frames shown
one refresh apart) and a window's survivors must arrive at least half a refresh later than it, or
the alternation is frames colliding on the deadline at the full-rate latency. The stretch is
remembered across the frame history because the one that matters can be seconds old: the morning
session's detection came 0.3 s into a gesture that followed a resize storm, with nothing clean in
between, after a drag that had run at full rate for seconds. A session that starts in Low Power
Mode never sees a clean stretch and is judged as before. On the three sessions that paced wrongly
the replay now makes no change; on the Low Power Mode logs it detects as before.

## CAMetalDisplayLink (2026-10-02)

A one-off probe (since removed; it lived in experiments/examples for a day) drove a bare
CAMetalLayer from a CAMetalDisplayLink (macOS 14+), rendered a clear into each update's drawable,
presented it, and reported what the link said against what the glass did. Six seconds in high
power on AC, WindowServer at 49% of a core from other apps:

| | default (preferredFrameLatency 2) | preferredFrameLatency 1 |
|---|---|---|
| callback interval | 8.33 ms | 8.33 ms |
| target timestamp, ahead of the callback | 8.2 ms (the next vsync) | 8.2 ms |
| target presentation, ahead of the callback | 41.5 ms (five frames) | 41.6 ms |
| presented against the promise | within 0.001 ms at p95 | within 0.004 ms |
| never presented | 10 of 702 | 3 of 708 |

So the system knows exactly when a frame will show and says so: the promise is kept to the
microsecond. But through this API the frame shows five refreshes after the callback, where our
tick path's frame shows three after its tick (paint to glass 24.4 ms), and the latency setting
did not move it. That is the figure Chromium's wrapper records in its comments, and Chromium does
not use the link in production. In Low Power Mode (next section) it kept calling at 120 Hz while
the server showed every other frame, so it does not know the server's rate either.

## Low Power Mode with a game running (2026-10-02)

The stress test: Low Power Mode on, FTL running, WindowServer at 50% of a core. The probe above
answered its question first: CAMetalDisplayLink kept calling at 120 Hz and promising presentation
41.5 ms out, and the server discarded 351 of 704 frames, every other one. The API does not know
the server's rate either; the pacer's inference has no replacement.

The replays passed: both flicks and the drag detected half rate within 0.3-0.9 s of the first
tick, read `OK_PACED` with 92-96% of shown intervals at two refreshes, and lost 0-1 frames once
paced. The editor session did not. It churned through divisors 4, 2, 3 and 3 between 168.7 and
170.9 s, with resets between, then held a third of the rate for 23 s. The log shows why: from
165 s the window was being displayed at 60 Hz with nothing changing and no resize, and once the
gesture began every tick's frame had a second paint 2-4 ms from it, before or after. Two paints
per refresh on a half-rate server discard one of each pair, the discarded one is often the
tick's, and the pattern reads as a quarter or a third of the rate. The second paints were not
ours: they carry no dirty rectangles, so Core Animation or AppKit displayed the layer on its own.
What drove that for five seconds is not in the log, so paints with no dirty rectangles are now
logged as kind `S` (the report counts them as "system repaints") and the next such session will
say.

The pacer's rule for it: a discarded frame that shared its refresh with a paint that was not a
tick's, within three quarters of a refresh before or after, was never going to show and is not
the server's discard, in the half-rate evidence or in the paced reset count. Two ticks' frames
that close are a late tick under load, still the server's to show or drop; counting those too
cost a Low Power Mode replay its detection. On this session's log the replay now makes no change,
and the Low Power Mode detections are unchanged. A rig test paints twice per tick on a full-rate
server.

## Which SDK the binary says it was linked against (2026-10-02)

AppKit keys its display behaviour on the SDK recorded in the executable's `LC_BUILD_VERSION`
load command, and on nothing else. That field can be rewritten in place (`vtool
-set-build-version macos 11.0 <sdk> -replace`, then re-sign with `codesign -s - -f`) and checked
with `vtool -show-build`, so the drag benchmark was run on 26.5.1 with its recorded SDK set to
nine values, on the AppKit-driven per-event path (`--per-event`: mark dirty, let Core Animation
display) and on the tick path (which displays the layer itself from the tick). Then the reverse:
the same source compiled against the system 26.5 SDK (`out/release-sdk26`) and patched down.
Under games' load, WindowServer at a third of a core. The discriminator is where the paints fall
within the refresh: spread across it when AppKit displays on request, clustered at one point
when it defers.

| built against | recorded SDK | per-event paint phase | paint to glass | never shown / late |
|---|---|---|---|---|
| 14 | 14.0, 15.0, 15.5, 15.6 | spread, about 5 ms wide | 20-23 ms | 29-61 / 17-51 |
| 14 | 16.0, 25.0, 26.0, 26.5 | clustered | 22-26 ms | see below |
| 26.5 | 14.0, 15.6 | spread, 4.9 ms wide | 20-21 ms | 30-32 / 18 |
| 26.5 | 16.0, 26.5 | clustered | 22-25 ms | 0-17 / 6-24 |

So the gate is any recorded SDK above 15.x, there being no SDKs between 15 and 26, and the
compiled-against SDK is irrelevant: a 26.5 build patched to 15.6 displays on request like a 14
build, and a 14 build patched to 16.0 defers like a 26 build. The tick path reads the same under
every label (paint 2.25 ms after the vsync, 22.7 ms to the glass), because it displays the layer
itself and AppKit's deferral never sees the request; `PX_EVENT_DISPLAY` paints the same way.

Where AppKit defers to is not fixed. Across launches an hour apart the display point sat at 6.5,
3.1, 2.7, 0.55 and 0.3 ms after the vsync, stable over three launches in a row and moved over
tens of minutes as the load changed. At 6.5 ms, 1.8 ms before the next vsync and on the window
server's early deadline from "The tick's phase", the deferred path was the worst thing measured:
93-111 frames never shown and 116-137 late out of 450. At 0.3 ms it was the best: 0-5 never
shown, 6-9 late, paint to glass 24.6 ms. Linking against the 26 SDK therefore does not buy
pacing; it buys AppKit's choice of phase, which is sometimes the deadline. That is what hit the
editor's 26-SDK builds on 2026-10-01, and it is the argument for keeping our own phase on every
OS: the hold and the tick-turn paint put the commit where we measured it safe, and the 2.25 ms it
lands at today is inside the window on both systems.

Corrections. "The SDK the binary is linked against changes this" above (2026-09-28) found the 26
SDK better for per-event painting and the 14 SDK worse; both are true, depending on where the
display point was that day. And `out/release/args.gn` has had `mac_sdk_path` set to the vendored
14 SDK since 2026-10-02, so the editor builds since then record SDK 14.0; the compile-time SDK
macro of a build agrees with its recorded SDK, and either would say 14 for those binaries.

What this means for a default that depends on the system: the SDK does not reach either scroll
path, so a gate between event and tick scrolling is the OS version at run time
(`@available(macOS 26.0, *)`), not the SDK. A compile-time SDK check is worth having only for
behaviours that go through AppKit's display cycle, which here means the resize path.

Done the same day. px exposes both facts, `px_os_smooths_event_frames()` (the running OS) and
`px_linked_against_macos_26_sdk()` (the SDK compiled against, from `__MAC_OS_X_VERSION_MAX_ALLOWED`,
which is the field AppKit reads in every binary nobody has rewritten); the editor paints scroll
and drag frames from the events where the first is true and from the display clock otherwise,
and its Cmd-4 overlay names the path and both facts on its first line. The environment switches
that selected these behaviours by hand are gone: `PX_EVENT_DISPLAY`, `SMOOTH_SCROLL_MODE`,
`PX_FRAME_PACER`, `PX_CA_DISPLAY_LINK`, `PX_METAL_PRESENT` and `PX_METAL_DRAWABLES`, and with
the last three the code paths they enabled (the CADisplayLink tick source and direct
presentation, both measured and rejected above). The scroll modes other than resample remain
reachable through `set_mode`, as the demo and the tests use them. Earlier sections name the
switches as they were used at the time.

Superseded 2026-10-02: the run-time gate and both query functions are gone. The split is
compile-time on the SDK (`PX_OS_SMOOTHS_EVENT_FRAMES`), since the SDK is what selects AppKit's
window and its paced display, and on the 26-SDK path the editor runs no display link at all.

## The always-on log

**Removed 2026-10-02** with the pacer; see "Pacing to the window server".

Every px program now writes `~/Library/Logs/px/<process>-<date>-<pid>.tsv` (px/mac/px_perf_log.h):
scroll input, ticks, paints, presented or discarded per frame, the pacer's mode changes, and the
application's per-frame note (the editor and the benchmarks log the scroll offset). Recording is a
fixed-size append on the main thread; a background queue formats and writes. On the Low Power
Mode replay the benchmark measured no difference with it on or off (paint p50 0.62 ms both ways).
`out/release/perf_report --latest` prints a one-line verdict and the numbers behind it; see
benchmark/README.md for the verdict words.

## On the glass against Sublime Text (2026-09-30, high power)

The same recorded flick posted into Sublime Text 4212 and into `scroll_demo` (the editor's font,
gutter and line geometry, nothing else), both at line 8000 of a 17 500-line source file, captured
from the display at 120 fps; see benchmark/README.md, "Measuring other apps". Travel is the net
movement of the text against the gesture's 7044 px; roughness is the median second difference of
single-refresh steps in the momentum phase, the number that reads as texture; held frames are
momentum frames shown two refreshes apart, which read as hitches.

| policy | travel | fps | step dev p50 | roughness median / p90 | held | lag to glass |
|---|---|---|---|---|---|---|
| Sublime Text | 100% | 100 | 2 px | 2 / 8 px | 10 | 11 ms |
| resample (the default until now) | 103% | 101 | 16.5 px | 8 / 100 px | 13 | |
| resample_keep | 100% | 102 | 22 px | 7 / 60 px | 15 | |
| accumulate | 100% | 91 | 7 px | 2 / 154 px | 19 | |
| accumulate_predict | 100% | 100 | 3 px | 2 / 8 px | 10 | 19 ms |

What it says: the display shows our painted positions faithfully (capture and log agree to the
pixel), so the wobble of the resampled policies is in the positions, and comes from fitting a line
through event timestamps that jitter while the deltas do not. Accumulating the deltas, as Sublime
does, removes it, but a tick that runs 3 ms before the vsync misses the display-locked event about
one time in six and then holds a frame and doubles the next (Chromium calls this the late-event
problem; it waits up to a third of a frame for the event, which our latch budget does not allow,
or predicts for the empty queue). Predicting one event's worth of motion only on a tick that found
no event, and carrying any lead the input never caught up with as a bias worked off in the next
motion, matches Sublime on every column but latency. The in-process replay agrees: input to
present 24 ms against 33, step error p50 3.7 pt against 4.8.

Latency is the remaining gap and it is structural: Sublime paints on the event, we paint on the
tick after it. Our per-event policy folds to 60 fps in high power (two events per paint, a paint
every 17 ms), which is why it is not the answer as it stands; the suspect is the
presentsWithTransaction commit lengthening each event's turn, unmeasured.

Also learned: the "13% short" travel seen first was the capture's search range (half the strip)
dropping a 1058 px frame, plus Sublime having clamped at the top of a short file; with the full
range every app reads 100%, and the resample policy's real fidelity error is a 3% overshoot from
resting past the input at stops.

## Latency: where Sublime's refresh came from (2026-09-30, late)

Breaking our input-to-glass path into stages with the always-on log and the capture: a scroll
event is delivered to the app about 7 ms after its HID timestamp; with the tick policies it then
waits for the next tick (median 4.6 ms); AppKit on this SDK paints at a display point about 4 ms
after the tick; and a painted frame takes two refreshes to reach the glass (the capture shows a
frame 7.8 ms before Core Animation's presentedTime, so presentedTime overstates it by one).
Sublime paints on the event's delivery instead of on the next tick, and that is the whole gap.

Things measured and ruled out, each worth not repeating: presenting the drawable from the command
buffer instead of with the transaction (lag 18 vs 19 ms, more jitter; and with per-event painting
it blocks the main thread 8 ms per paint in nextDrawable and halves the input delivered); a
two-drawable pool (19 ms); drawing inside the tick's turn (a frame every other tick, 60 ms lag);
the OpenGL layer, Sublime's own layer type (20 ms); and painting on the event by marking the layer
dirty and calling displayIfNeeded (a second, scheduled display pass repaints the same content and
supersedes the early frame: 17 ms).

What works is `px_display_now`: the event handler drains the dirty list straight into the layer's
display, with no deferred pass left behind. Per-event painting that way measured 9 ms on the
glass against Sublime's 11, at 103 fps with Sublime's step quality. The `hybrid` policy keeps it
together with everything else: at full rate each event is accumulated exactly and painted in its
own turn (`smooth_scroll::present_input`), the display link stays alive but its ticks only keep
the gesture's bookkeeping (`set_event_driven`); when the frame pacer halves the ticks, events stop
painting and the paced ticks paint with accumulate_predict as before. A first version that also
let ticks predict at full rate lost a fifth of its frames, because the tick fires 3 ms before the
vsync and the event is delivered after it, so the prediction was always one step ahead of an
event that then had nothing new to show.

| high power, same flick | Sublime Text | hybrid |
|---|---|---|
| travel | 100% | 100% |
| fps while moving | 100 | 101 |
| intervals at 1 refresh | 361 | 368 |
| step deviation p50 / p95 | 3 / 188 px | 2 / 177 px |
| momentum held / roughness med / p90 | 10 / 2 / 8 px | 10 / 2 / 6 px |
| input to glass | 11 ms | 9 ms |

The editor ran this path by default for part of 2026-10-01. After the macOS 15.5 results below
the default returned to resample, which holds on both systems; `SMOOTH_SCROLL_MODE=predict`
selects this path.

`scroll_benchmark --replay` drives the same path since 2026-10-01 (it ran accumulate_predict from
the ticks alone before, a path the editor never uses), and `benchmark/check.sh` replays the
checked-in flick and drag traces through it. On the flick trace the hybrid reads worse than the
editor's own field logs on the same path: input to present 38-39 ms against 28 ms, a quarter of
the frames a refresh later than usual, 59-64 of 505 paints never shown. Nearly all of the
unshown frames are the app's own doing: the trace has 240 Hz event pairs for 15% of its intervals
(a finger drag; tonight's real sessions had 4-7%), each pair paints twice in one refresh, and the
server shows the second (56 of 63 discards had another paint within a refresh; the report tool
counts those as superseded, not against the server). The latency and the late frames come from
the second paint landing mid-refresh, and in a sustained 240 Hz burst from the three-drawable pool
running dry (acquire waits to 6 ms on 15 of the 76 late frames). The real sessions showed none of
it. Skipping the second paint of a pair and letting the tick paint that refresh is the candidate
fix, unmeasured.

In Low Power Mode the hand-off works: half rate is detected 167 ms into the gesture, after which
the paced ticks paint every shown refresh. Against Sublime at 60 Hz the hybrid is smoother
(momentum roughness 2 / 5 px against 2 / 8, zero held frames, 100% travel, 13 discards all before
the hand-off) and 9 ms slower to the glass. Painting on events at half rate instead was measured
and is worse for us: at the display point it shows the parity flips (roughness 12 / 374 px);
drawn in the event's turn it commits two frames per shown refresh, the server discards half, the
pacer churns, and it is no faster. The remaining 9 ms at half rate is not the policy: the same
hybrid on the OpenGL layer (`PX_GL=1`) measures 19 ms there against Sublime's 18, so the Metal
layer's transaction presentation holds a frame one 60 Hz slot longer at half rate (in high power
the two layers are equal). Presenting the drawable directly makes it far worse at half rate
(60 ms: the queue fills), and a two-drawable pool blocks the main thread in `nextDrawable` for
16 ms per paint. That one slot is the open item.

## Live events on the 26-SDK window (2026-10-02)

The per-event path of the compile-time split felt "insanely laggy" in `out/release-sdk26/editor`
while `scroll_benchmark --replay` of the same binary looked smooth and Sublime Text 4212 (also
linked against the 26.5 SDK) scrolled smoothly beside it, with a game and a video running.
`scroll_benchmark --record` now prints the replay's report for the live gesture, and
`drive_scroll --pid` posts a recorded trace at the HID tap into the recording window, so the same
flick (benchmark/traces/flick.tsv) was measured through AppKit's event path and through the
in-process timer, on one binary, under the same load:

| run | path into the app | events / paint | paint interval p50 | shown of painted | input to glass p50 |
|---|---|---|---|---|---|
| replay | main-queue timer | 1 | 8.3 ms | 463 / 470 | 26 ms |
| live, Metal | AppKit, observer-only flush | 2 | 16.7 ms | 233 / 235 | 30 ms |
| live, OpenGL (`PX_GL=1`) | same | 2 | 16.7 ms | (no telemetry) | |
| live, Metal, in an .app bundle | same | 2 | 16.7 ms | 235 / 237 | 34 ms |
| live, 14-SDK build | AppKit, observer-only flush | 1 | 7.8 ms | 397 / 495 | 25 ms |
| live, Metal, flush in send_event (x4) | AppKit | 1 | 8.3 ms | 449/462, 451/458, 422/466, 267/465 | 30-32 ms |
| Sublime Text 4212, glass capture | its own | | 8.3 ms (308 of 350 intervals) | | |

With the observer-only flush the 26-SDK window woke the main thread for scroll events two at a
time, every other refresh: the received events were 8.3 ms apart by their own timestamps, but the
run-loop probe (`run_loop_busy` / `run_loop_sleep`, a passive observer in the benchmark) showed the
thread asleep for up to 9 ms with an event already queued and never busy for more than 3 ms. So
nothing of ours blocked; AppKit delivered in pairs, the app painted once per pair, and the glass
ran at 60 Hz with 7 ms more latency. Neither the layer type, nor an app bundle, nor the display
link changed it, and the 14-SDK window never did it. Marking the layer dirty inside
`send_event`, before `sendEvent:` returns (ST's post-condition, which px had dropped on
2026-09-07 as redundant for resize), makes AppKit deliver one event per refresh and display it in
that turn: paints equal events at 8.3 ms in every run. Why AppKit keys its wake-ups on that is
not known; the measurement is the rule.

What remains is the window server's: in one of five flush runs 43% of the frames were discarded
at a steady 16.7 ms cadence, the commit-on-the-deadline collision of "The tick's phase", which
AppKit's per-launch display phase decides and the editor no longer tries to steer. Sublime's
capture under the same drive had 36 two-refresh intervals in 350. The thumb drag was not driven
live (no mouse-drag poster exists); its events take the same route, and
`scrollbar_benchmark --record` reports a hand drag.

## Measuring

Build the benchmark and record a trace by scrolling its window yourself (Escape when done):

```sh
bin/ninja -C out/release scroll_benchmark
out/release/scroll_benchmark --record /tmp/flick.tsv
```

The trace is every scroll event the window received, timed from the first one, with the delta px
delivered. `--replay` plays it back into the same scene from a mach-clock thread with a chosen
phase against the display link and reports `presentedTime` feedback:

```sh
out/release/scroll_benchmark --replay /tmp/flick.tsv [--input-phase-ms 4] [--fullscreen] [--dump-frames]
```

`presentation_interval` should sit at the refresh interval with a p99 no worse than one missed
refresh, `dropped_presentations` (drawables that never reached the glass) should be in the single
digits, and `presented_step_error` is the smoothness number; `presented_position_error` and
`input_to_present` are the price paid in lag. `target_to_present` and `tick_delay` separate the
window server's latency from the app's.

## Recording your own scrolling

Recording and replay run in one process. Replayed events carry their recorded timestamps, so the
app sees the device's timing rather than the replayer's. An earlier version replayed through the
HID tap into the editor, with a Python report over the editor's stderr log; it reproduced the
in-process numbers while costing Accessibility access, a warped pointer and a global side effect
on whatever sat under it, so it was removed on 2026-09-29. The trace carries no scroll phases: px
does not deliver them and `smooth_scroll` does not read them.

A smooth run shows `presentation_interval` at the refresh interval on every frame during the
gesture and a `presented_step_error` near zero; a repeated step of 0 followed by a double step is
the beat pattern.

macOS changes window compositing and ProMotion cadence with visibility and system load. Check
`top -o cpu` for WindowServer and browsers before trusting a run, and run each configuration
several times.

## Scrollbar dragging

A thumb drag goes through the same pipeline as a gesture, with `smooth_scroll::track` in place of
`scroll`: each mouse event records the offset the pointer asks for, and the display link draws one
frame per refresh from the pointer's trajectory, sampled 9.5 ms behind the tick. Unlike a gesture
the trajectory ends exactly on the latest target, since the thumb has to come to rest under the
pointer: the sample never runs past it, and once there the ticks go idle.

It used to jump on every event instead (`jump_to` and `mark_dirty` per `mouseDragged`), one commit
per mouse event at whatever phase the mouse had against the display, with the display link off.
Measured on 2026-09-28 with a 4.2 s trackpad drag in a maximized window: input at 120 Hz, every
event painted (0.16 ms each), and of 464 commits the window server never displayed 46 and showed
31 a refresh late. Each pair is a held frame followed by a double step, about twelve times a
second. The steps of the frames that did land were half a step off the pointer's trajectory
(`presented_step_error` p50 23 pt against a 47 pt step), since consecutive trackpad deltas are
uneven and nothing resampled them. That path is kept as `scrollbar_benchmark --per-event` for
comparison.

**The SDK the binary is linked against changes this.** Measured 2026-09-28 on macOS 26.5 with the
same trace, the per-event path, and identical code: linked against the vendored macOS 14.0 SDK
(`mac_sdk_path` set in args.gn), every paint follows its event within 0.6 ms and the window server
never displays 107 of 464 frames and shows 97 a refresh late. Linked against the system 26.5 SDK
(`mac_sdk_path` omitted), AppKit defers the layer's display to a vsync-locked point once per
refresh: paints land 1-9 ms after their event at 8.33 ms intervals, paint-to-present is constant to
0.15 ms, same-refresh events fold into one frame, and only 8 frames are never displayed and 14
late. So on a 26-SDK build the per-event path already behaves like Chromium's model, at about 2 ms
more paint-to-present; the tick path adds the resampling on top and behaves the same on both SDKs,
which is why it stays. Builds before 2026-09-27 were linked against the 14.0 SDK, so every
measurement above this section was taken under the first behaviour; `out/release-sdk14` (same
args plus the vendored `mac_sdk_path`) reproduces it.

`scrollbar_benchmark` measures either path with the editor's own scene and drag code and a
recorded drag as input:

```sh
bin/ninja -C out/release scrollbar_benchmark
benchmark/record_scrollbar_drag.sh slow       # drag the thumb; Escape in the window when done
out/release/scrollbar_benchmark --replay /tmp/scrollbar-drag-slow.tsv
out/release/scrollbar_benchmark --replay /tmp/scrollbar-drag-slow.tsv --per-event
```

The recording is the drag as the benchmark received it (press, every pointer position while the
button was held, release), in window points, with the window and font size it happened in; a
replay reproduces both so the press lands on the thumb where it was. The replay runs from a timed
thread inside the process: it is deterministic and reproduced the recording's numbers, as did a
replay through the HID tap that has since been removed. Recording and replay print the same
report:

- `event_interval` is the mouse's own cadence and `event_lag` how long each event took to reach
  the handler. `events_per_frame` is 1 when frames follow events; on the tick path it is 1 with
  the odd 0 and 2 where the mouse's clock drifts against the display's, which is the beat being
  absorbed rather than shown.
- `presentation_interval` should sit at the refresh interval; `late_frames` and `never_displayed`
  are the frames that did not. `presented_step` is the content motion per displayed frame and
  `presented_step_error` its deviation from the pointer's trajectory sampled at the frame's
  presentation time, which is the smoothness number: on a perfectly paced drag it is near zero,
  on a beat it is a full step. `irregular_steps` counts the frames the report tool's rule would
  flag (a step more than 35% off the mean of its neighbours), and the worst are listed.
- `input_to_present` and `presented_position_error` are the lag, in time and in points. The tick
  path pays the resample latency here, as the gesture path does.
- `content_per_pointer_pt` is how far the document moves per point of pointer travel. With 500
  lines it is around ten, so every pointer step is a visible jump even when the cadence is
  perfect.

### Newest position per tick (2026-10-01)

The resampled drag above felt smooth on both macOS 26.5 and 15.5 but left the thumb visibly
behind the cursor: `presented_position_error` p50 152-178 content points on the checked-in drag,
about 15 points of pointer travel, where the per-event path (`--per-event`) measured 134-138.
Shortening the resample latency to 5 ms closed most of that (136-142) and cost a third more
irregular steps, because a drag's sample is clamped to the pointer's last position and the
latency is the only headroom a tick has when its event is late. Per-event dragging by hand on
26.5 read as tight but laggy in cadence (it drops frames there) and is the path that misses the
15.5 server's latch.

Chromium's thumb drag does neither: `cc/input/scrollbar_controller.cc` applies one pointer
position per frame with no resampling or prediction, and documents that applying two in a frame
made the thumb jitter. `smooth_scroll::track` now does the same, the newest pointer position
shown on the next tick. It is tick-painted, so it holds on 15.5, and as tight as per-event. The
cost is the beat between the mouse's rate and the display's: a tick that finds no new position
holds its frame and the next shows a doubled step, which the resampler used to absorb.

On the checked-in drag, three replays (one disturbed by the server dropping to half rate
mid-run, on battery): `presented_position_error` p50 115-123 content points, about 10 points of
pointer travel and the tightest of every path measured; two-refresh holds 17-21 per 4.2 s drag
against 7-11 on the resampled path, and irregular steps 47-53 against 19-35. That is the beat
made visible, four or five times a second on a trackpad drag whose event intervals run to
9.9 ms at p95 and 12.7 ms at p99. A middle path exists and is unmeasured: `accumulate_predict`
applied to the drag, the newest position when a tick has one and one predicted step when it does
not, never moving against the pointer, resting at most one event's motion from under it.
