# Fontconfig

Vendored fontconfig (submodule `src/`, v2.18.2). Built on **Linux** (production)
and **macOS** (for debugging the FreeType/Fontconfig path off its native
platform). In production, macOS and Windows resolve fonts through CoreText /
DirectWrite. Windows is not ported (`assert(is_linux || is_mac)`); it would need
fontconfig's dirent/mkstemp emulation and registry font dirs.

- Font-file backend: FreeType (`//third_party/freetype`), not the Rust
  "fontations" path — so `fcfreetype.c` is compiled and `fcfontations.c` is not.
- XML config parser: **expat**, vendored at `//third_party/expat` (built from
  source on both platforms; no iconv/ICU, unlike libxml2). Leaving
  `ENABLE_LIBXML2` undefined selects expat in `fcxml.c`.

Fontconfig normally generates a handful of headers during its meson build. We
don't run meson; instead the generated headers are produced once with
fontconfig's own scripts and checked in. Regenerate them after bumping `src/`.

## Checked-in generated files

| File | Produced by |
| --- | --- |
| `fcstdint.h` | copy of `src/src/fcstdint.h.in` |
| `include/fontconfig/fontconfig.h` | `fontconfig.h.in` + cache versions |
| `include/src/fclang.h` | `fc-lang.py` (orthography tables) |
| `include/fc-case/fccase.h` | `fc-case.py` (case-folding table) |
| `include/src/fcobjshash.h` | preprocess → `cutout.py` → `gperf` |
| `include/src/fcconst.h` | `fc-const.py` |
| `include/src/fcgenericfamily.h` | `fc-genericfamily.py` → `gperf` |
| `include/src/fc{,ft}alias{,tail}.h` | empty (see note) |
| `config.h` | hand-maintained (see note) |

## Regenerating

Run `./generate_headers.sh` (needs `python3`, `gperf`, and a C compiler). It
reproduces every row of the table above by driving fontconfig's own generator
scripts, reading the cache versions and orthography-file order from the
submodule so a version bump flows through automatically. It does **not** touch
`config.h` (hand-maintained, below).

After a `src/` submodule bump: run the script, then rebuild all targets
(`./scripts/build-all.sh`) — a new upstream version surfaces any missing config
knob as a compile `#error` or link failure. The script also warns if the gperf
version it ran disagrees with `config.h`'s `FC_GPERF_SIZE_T`.

### Empty alias headers

`fc{,ft}alias.h` / `fc{,ft}aliastail.h` normally give exported symbols hidden
`IA__` aliases so a **shared** library can call them without PLT indirection.
This is a static library, so the aliasing is pointless: leaving the four
headers empty makes `#include "fcaliastail.h"` a no-op and every function keeps
its normal name. (Consistency matters — both the head and tail headers must be
empty, or both populated.)

### config.h

Hand-maintained because we don't run fontconfig's meson/configure. A common
block covers both platforms (both 64-bit little-endian), with a
`#if defined(__APPLE__)` / `#else` split for the differences — macOS has no
`random_r`, `<sys/vfs.h>`, or `<sys/statfs.h>` (it uses `<sys/mount.h>` +
`<sys/param.h>`, with the fs-type name in `struct statfs.f_fstypename`), and
different default font paths. A `_WIN32` section could be added later. It
mirrors a meson-configured 2.18.2 build with these deliberate deltas:

- No NLS: `ENABLE_NLS` unset (no gettext/libintl dependency).
- expat backend: `ENABLE_LIBXML2` unset.
- FreeType backend: `ENABLE_FREETYPE 1`, `ENABLE_FONTATIONS` unset.
- `FC_GPERF_SIZE_T unsigned int` — must match the `len` type in the gperf
  output. The macOS host gperf (3.0.x) emits `unsigned int`; a gperf ≥ 3.1
  emits `size_t`. Keep this in sync with `include/src/fcobjshash.h`.
- Only the `HAVE_FT_*` optional APIs whose implementation is compiled in
  `//third_party/freetype` are enabled (no BDF properties, no X11 font-format
  query).

`_GNU_SOURCE` and `HAVE_CONFIG_H` are set in `BUILD.gn` (not `config.h`) so
`_GNU_SOURCE` precedes any libc header regardless of include order.
