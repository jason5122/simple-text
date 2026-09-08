# Sublime Text Linux Renderer Notes

Status: the CPU/Skia path matches all 672 Sublime Text CPU-renderer reference captures
pixel-for-pixel. The OpenGL path mirrors Sublime's recovered GPU shader and blend state; it cannot
be compared directly with Sublime's GPU output in the current VM.

Binary: Sublime Text Build 4200, Linux x86-64, captured in the Fedora ARM64 VM

Last updated: 2026-09-07

## Parallels OpenGL limitation

The Fedora 42 ARM64 Parallels VM's default capability report is:

```text
OpenGL renderer: virgl (Apple M4 Max (Compat))
OpenGL version: 4.0 (Compatibility Profile) Mesa 25.1.4
GLSL version: 4.00
```

When the local GDK backend requests its OpenGL 3.3 feature floor, the same driver realizes a 4.0
core context (`gdk_gl_context_is_legacy() == false`). The shared GLSL 3.30 shaders compile there and
the local OpenGL path runs successfully.

This is a virtual-GPU capability limit rather than a missing Mesa package. Sublime Text's Linux GL
shaders request GLSL 4.10, so Sublime exits with the following error before its GPU renderer can be
captured in this VM:

```text
Failed to initialize OpenGL rendering: GLSL 4.10 is not supported.
```

The limitation prevents a direct pixel comparison against Sublime's GPU output. The available
reference captures come from Sublime's CPU renderer, whose byte-level compositing differs from
hardware RGBA8 blending.

## Current OpenGL path

The recovered `glyph_subpixel` fragment shader emits the tint and per-channel coverage as
dual-source outputs. Disassembly of Build 4200 confirms that its draw path selects
`glBlendFunc(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR)` and restores
`glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA)` afterward. It does not read the framebuffer or
perform integer source-over in the shader.

The local renderer now follows the same design on every platform:

1. Batch adjacent glyphs that use the same atlas page and shader mode while preserving submission
   order for overlapping marks and layered emoji.
2. Emit tint and coverage from the shared fragment shader.
3. Use dual-source fixed-function blending for monochrome glyphs and premultiplied-alpha blending
   for intrinsic-color glyphs.

There is no Linux shader define, destination scratch texture, framebuffer copy, or per-glyph draw
loop. The CPU/Skia renderer remains the Linux default; set `PX_USE_GL=1` to select OpenGL.

## Real-hardware follow-up

On real Linux hardware, first record the native driver and extension set:

```sh
glxinfo -B
```

GLSL 4.10 or newer should allow Sublime's own GPU renderer to start. Capture that output before
drawing conclusions from differences against the CPU oracle; fixed-function blend quantization is
driver and framebuffer dependent.

## Conformance commands

Run from `out/linux-arm64-release` on the macOS host:

```sh
PX_USE_GL=1 LINUX_OUR_OUTPUT=/media/psf/linux-arm64-release/ours-gl ./capture-ours-vm.sh
OURS_DIR=ours-gl DIFF_RESULTS_DIR=diff-gl ./diff.sh
```

The comparison against Sublime's CPU-renderer captures recorded on 2026-09-07 was:

```text
Correct:       278
Small diffs:   160
Large diffs:   234
```

"Large" means more than 200 affected pixels, not a visually large error. Differences are generally
edge quantization; the maximum observed per-channel delta was 6/255. This result must not be used
as evidence that Sublime's GPU renderer performs the CPU renderer's integer rounding.
