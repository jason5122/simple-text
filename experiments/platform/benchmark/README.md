# Drag latency benchmarks

This directory contains the maintained end-to-end benchmarks for the macOS
input-to-presentation path and the equivalent Sublime Text sidebar drag. Both use the same mouse
driver, recording settings, analysis model, and output format.

From the repository root, build the complete benchmark with:

```sh
bin/ninja -C out/release benchmarks
```

Then run:

```sh
experiments/platform/benchmark/record_platform_drag.sh 200
experiments/platform/benchmark/record_sublime_text_drag.sh 200
```

Each script records four seconds, drives ten high-speed vertical round trips with `move_mouse`, and
immediately reports the same cursor-lead, inferred-lag, and sampled-speed distributions. An
optional second argument chooses the output `.mov` path.

The Sublime benchmark uses the original reproducible crop and coordinates: its sidebar must have a
draggable folder row at screen position `(80, 130)`. Set `SUBLIME_TEXT_APP` to benchmark a copy
outside `/Applications/Sublime Text.app`.

The animation benchmark isolates frame pacing from the demo's layout and text. It draws one
constant-speed bar over a flat background while injecting a fixed amount of scroll input. The third
argument adds same-colored rectangles to each frame without changing the captured image:

```sh
experiments/platform/benchmark/record_animation_scroll.sh large 4000 0
experiments/platform/benchmark/record_animation_scroll.sh large 4000 256
experiments/platform/benchmark/record_animation_scroll.sh large 4000 512
experiments/platform/benchmark/record_animation_scroll.sh large 4000 1024
```

Each accepted run reports `scroll_input_complete`, then measures the bar's position error,
per-frame step error, and observed speed.

To measure the same bar while the copied demo scrolls its real rows and text in a clipped lower
viewport, run:

```sh
experiments/platform/benchmark/record_demo_isolation_scroll.sh large 4000 48 all
```

This records three otherwise-identical cases: content fully offscreen, content visible but fixed,
and content visible and scrolling. The differences isolate screen-recording variance, visible
content rendering, and scrolling/culling work respectively. Each analysis scans only the top 96
points, above the document clip. Pass one of `offscreen`, `onscreen`, `lower`, or `normal` instead
of `all` to record a single case. A following `rectangles` or `text` argument draws only that part
of the document, allowing the fixed onscreen case to isolate the two rendering paths. Use
`text-cached` or `all-cached` to reuse layouts shaped before animation starts while preserving the
same glyph rendering work.
