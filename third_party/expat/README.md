# expat

Vendored [libexpat](https://github.com/libexpat/libexpat) (submodule `src/`,
tag `R_2_8_2`). It's the XML parser fontconfig uses to read `fonts.conf` — we
build it from source (rather than linking the sysroot/SDK `libexpat`) so the
FreeType/Fontconfig path builds the same way on Linux and macOS. Unlike
libxml2, expat needs no iconv/ICU.

## Build shape

`BUILD.gn` compiles the three core translation units plus one entropy backend:

- `xmlparse.c`, `xmlrole.c`, `xmltok.c` (`xmltok.c` `#include`s
  `xmltok_impl.c` / `xmltok_ns.c`, so those aren't listed separately)
- `random_getentropy.c` — selected by `HAVE_GETENTROPY` in `expat_config.h`.
  `getentropy()` exists on glibc ≥ 2.25 (the bullseye sysroot is 2.31) and
  macOS ≥ 10.12, so one backend covers both targets.

## expat_config.h

expat's sources `#include "expat_config.h"` unconditionally; upstream generates
it via CMake/autoconf. We don't run those, so it's **hand-maintained** at the
port root. It targets 64-bit little-endian POSIX (macOS + Linux) with expat's
standard feature defaults (`XML_DTD`, `XML_GE 1`, `XML_NS`,
`XML_CONTEXT_BYTES 1024`). To refresh it after bumping `src/`, compare against
`src/expat/expat_config.h.cmake` and adjust any changed keys.

A Windows build would use expat's bundled `winconfig.h` instead and the
`random_rand_s.c` backend; that's not wired up (fontconfig isn't ported to
Windows yet).
