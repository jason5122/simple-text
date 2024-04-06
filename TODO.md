# TODO

## Priority

### UI Framework

1. Implement `onKeyDown(...)`.
   - Windows: Don't use accelerator tables (Chromium doesn't either).

### Linux

1. Load GTK 3 using `dlopen()` instead of adding it as a link-time dependency.

## Unordered

- Measure performance.
- Consider switching to Bazel
  - [Shaka Player discussion](https://github.com/shaka-project/shaka-player-embedded/issues/19) regarding this subject
