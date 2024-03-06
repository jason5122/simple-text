# TODO

## Priority

### Linux

1. Vendor libepoxy.
2. Ensure HarfBuzz is as lightweight as possible to reduce startup times.
3. Statically link against as many libraries as considered reasonable.
4. Load GTK 3 using `dlopen()` instead of adding it as a link-time dependency.

## Unordered

- Measure performance.
- Consider switching to Bazel
  - [Shaka Player discussion](https://github.com/shaka-project/shaka-player-embedded/issues/19) regarding this subject
- Add delegates to Objective-C code.
