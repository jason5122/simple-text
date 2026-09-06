# Sublime Text Linux Renderer Notes

Status: the CPU/Skia and OpenGL conformance paths both match all 672 Sublime Text reference
captures pixel-for-pixel.

Binary: Sublime Text Build 4200, Linux x86-64, captured in the Fedora ARM64 VM

Last updated: 2026-09-05

## Parallels OpenGL limitation

The Fedora 42 ARM64 Parallels VM currently exposes:

```text
OpenGL renderer: virgl (Apple M4 Max (Compat))
OpenGL version: 4.0 (Compatibility Profile) Mesa 25.1.4
GLSL version: 4.00
```

This is a virtual-GPU capability limit rather than a missing Mesa package. Sublime Text's Linux GL
shaders request GLSL 4.10, so Sublime exits with the following error before its GPU renderer can be
captured in this VM:

```text
Failed to initialize OpenGL rendering: GLSL 4.10 is not supported.
```

The VM also does not advertise the destination-read and synchronization facilities relevant to a
fast exact compositor, including shader image load/store, texture barriers, framebuffer fetch, and
fragment-shader interlock. The local shaders use only GLSL 4.00 so the reconstructed GL path can
still run in the VM, but it cannot use a single-pass destination-read implementation there.

Consequently, the VM blocks two kinds of follow-up work:

- Direct pixel comparison against Sublime's own GPU output. The current GL oracle is Sublime's
  pixel-perfect CPU output.
- Development of the preferred batched exact-compositing path using modern destination access.

It does not block correctness work: the compatibility implementation is 672/672 pixel-perfect.

## Current exact compatibility path

Ordinary RGBA8 fixed-function blending is insufficient. The VM rounds blend results to the nearest
byte, while Sublime's Linux monochrome compositor mostly divides by 255 exactly but rounds up when
the product is one below a multiple of 255. Intrinsic-color glyphs use exact division without that
boundary adjustment. Most of these byte mappings cannot be represented by changing only the GL
source color and blend factors.

The Linux GL renderer therefore uses this compatibility algorithm:

1. Preserve glyph submission order instead of regrouping nonadjacent atlas batches.
2. Copy the framebuffer rectangle covered by the next glyph into an RGBA8 scratch texture with
   `glCopyTexSubImage2D`.
3. Sample the glyph atlas and copied destination in the fragment shader.
4. Perform Sublime's recovered source-over operation with unsigned integer arithmetic.
5. Draw one glyph before copying the destination for the next glyph.

All rendering data stays on the GPU; there is no GPU-to-CPU readback. The cost is nevertheless
significant compared with a conventional text batch:

- one GPU-to-GPU region copy per glyph
- one draw call per glyph
- a serial copy/draw dependency to preserve overlapping-glyph order
- one additional framebuffer-sized RGBA8 scratch texture, although only glyph rectangles are
  copied into it

This path prioritizes conformance over throughput. The CPU/Skia renderer remains the Linux default;
set `PX_USE_GL=1` to select OpenGL.

## Real-hardware follow-up

On real Linux hardware, first record the native driver and extension set:

```sh
glxinfo -B
glxinfo -l | rg 'shader_image_load_store|texture_barrier|framebuffer_fetch|fragment_shader_interlock'
```

GLSL 4.10 or newer should allow Sublime's own GPU renderer to start, making a direct CPU-versus-GPU
Sublime capture possible. For our renderer, investigate these optimizations in increasing order of
scope:

1. Use a texture barrier to read the attached color texture between ordered glyph draws, eliminating
   the region copies while retaining the current integer shader.
2. Use shader image load/store with explicit barriers, also eliminating the scratch copies.
3. If the hardware supplies a suitable ordered framebuffer-fetch or fragment-interlock mechanism,
   restore multi-glyph batching while retaining exact integer source-over behavior.
4. Keep the GL 4.0 copy-based implementation as the compatibility fallback and compare both paths
   against the same 672-image suite.

Do not replace the integer compositor with fixed-function blending as an optimization: the latter
was the source of the remaining one-to-three-byte differences.

## Conformance commands

Run from `out/linux-arm64` on the macOS host:

```sh
PX_USE_GL=1 LINUX_OUR_OUTPUT=/media/psf/linux-arm64/ours-gl ./capture-ours-vm.sh
OURS_DIR=ours-gl DIFF_RESULTS_DIR=diff-gl ./diff.sh
```

The result recorded on 2026-09-05 was:

```text
Correct:       672
Small diffs:     0
Large diffs:     0
```
