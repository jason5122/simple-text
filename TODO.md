# TODO

## Priority

### UI Framework

- Reuse `//renderer` classes across windows.
- Mac: Implement CGL context sharing.
- Win: Keep track of windows being closed through `WM_DESTROY` instead of during keybinds.

### Linux

1. Load GTK 3 using `dlopen()` instead of adding it as a link-time dependency.

## Unordered

- Measure performance.
- Consider switching to Bazel
  - [Shaka Player discussion](https://github.com/shaka-project/shaka-player-embedded/issues/19) regarding this subject
