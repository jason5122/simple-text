# Drag latency benchmarks

This directory contains the maintained end-to-end benchmarks for the macOS
input-to-presentation path and the equivalent Sublime Text sidebar drag. Both use the same mouse
driver, recording settings, analysis model, and output format.

From the repository root, build the complete benchmark with:

```sh
bin/ninja -C out/release platform_benchmark_suite
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
