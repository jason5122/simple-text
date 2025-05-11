# macOS Startup Benchmark

This experiment is used to benchmark the startup time of a bare macOS app. The goal is to prevent startup times of the main app from regressing too much.

There are two modes:

- `NSView` (non-OpenGL)
- `CAOpenGLLayer`

## Benchmarks

```bash
hyperfine --warmup 20 --runs 50 '"./macOS Startup Benchmark"'
```

Device: MacBook Pro 16-inch (2023), Apple M2 Max

| Mode            | Time (ms)  |
| --------------- | ---------- |
| `NSView`        | 64.4 ± 1.7 |
| `CAOpenGLLayer` | 73.8 ± 2.4 |
