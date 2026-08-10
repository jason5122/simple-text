/*
 * Hand-maintained fontconfig configuration for the vendored build.
 *
 * In production fontconfig is built only on Linux, but this config also
 * supports macOS so the FreeType/Fontconfig path can be built and debugged
 * there (macOS/Windows production font handling uses CoreText/DirectWrite).
 * Both platforms are 64-bit little-endian (LP64). The values mirror a
 * meson-configured fontconfig 2.18.2 build; see README.md for the deltas and
 * for how the companion data-table headers are regenerated.
 *
 * A `#elif defined(_WIN32)` section could be added later for a Windows port
 * (fontconfig ships its own dirent/mkstemp emulation and uses registry font
 * dirs); it's intentionally omitted for now.
 */

#ifndef FC_SIMPLE_TEXT_CONFIG_H
#define FC_SIMPLE_TEXT_CONFIG_H

/* ---- Common to all supported platforms ---- */

/* Version 2.18.2. */
#define FC_VERSION_MAJOR 2
#define FC_VERSION_MINOR 18
#define FC_VERSION_MICRO 2

/* Font backend: FreeType, not the Rust "fontations" path. */
#define ENABLE_FREETYPE 1

/*
 * Type of the length parameter in the gperf-generated FcObjectTypeLookup().
 * The macOS host gperf (3.0.x) emits `unsigned int`; a gperf >= 3.1 emits
 * `size_t`. This must match include/src/fcobjshash.h -- keep the two in sync
 * whenever that header is regenerated.
 */
#define FC_GPERF_SIZE_T unsigned int

/*
 * Optional FreeType APIs. Only the ones whose implementation is in
 * //third_party/freetype's source list are enabled. BDF properties (ftbdf.c)
 * and the X11 font-format query (ftfntfmt.c) are not compiled there, so those
 * stay off and fcfreetype.c takes its fallback paths.
 */
#define HAVE_FT_GET_PS_FONT_INFO   1
#define HAVE_FT_HAS_PS_GLYPH_NAMES 1
#define HAVE_FT_DONE_MM_VAR        1

/* Atomics: C11 <stdatomic.h>, with the GCC/Clang __sync builtins as fallback. */
#define HAVE_STDATOMIC_PRIMITIVES    1
#define HAVE_INTEL_ATOMIC_PRIMITIVES 1

/* Flexible array members are supported (C99). */
#define FLEXIBLE_ARRAY_MEMBER /**/

/* ABI: 64-bit little-endian. */
#define SIZEOF_VOID_P   8
#define ALIGNOF_VOID_P  8
#define ALIGNOF_DOUBLE  8
#define WORDS_BIGENDIAN 0

/* No iconv; fcfreetype.c uses its built-in conversions. */
#define USE_ICONV 0

/* Headers present on both macOS and Linux. */
#define HAVE_DIRENT_H      1
#define HAVE_FCNTL_H       1
#define HAVE_STDINT_H      1
#define HAVE_STDLIB_H      1
#define HAVE_STRING_H      1
#define HAVE_STRINGS_H     1
#define HAVE_INTTYPES_H    1
#define HAVE_UNISTD_H      1
#define HAVE_DLFCN_H       1
#define HAVE_WCHAR_H       1
#define HAVE_TIME_H        1
#define HAVE_SYS_STAT_H    1
#define HAVE_SYS_TYPES_H   1
#define HAVE_SYS_PARAM_H   1
#define HAVE_SYS_MOUNT_H   1
#define HAVE_SYS_STATVFS_H 1

/* Functions / struct fields present on both. */
#define HAVE_GETPAGESIZE           1
#define HAVE_GETPID                1
#define HAVE_LINK                  1
#define HAVE_LSTAT                 1
#define HAVE_MKDTEMP               1
#define HAVE_MKSTEMP               1
#define HAVE_MMAP                  1
#define HAVE_RANDOM                1
#define HAVE_RAND_R                1
#define HAVE_LRAND48               1
#define HAVE_READLINK              1
#define HAVE_STRERROR              1
#define HAVE_STRERROR_R            1
#define HAVE_LOCALTIME_R           1
#define HAVE_FSTATFS               1
#define HAVE_FSTATVFS              1
#define HAVE_STRUCT_STATFS_F_FLAGS 1
#define HAVE_STRUCT_DIRENT_D_TYPE  1
#define HAVE_VPRINTF               1
#define HAVE_VSNPRINTF             1
#define HAVE_VASPRINTF             1
#define HAVE_C99_VSNPRINTF         1
#define HAVE_USELOCALE             1
#define HAVE_PTHREAD               1

/* ---- Platform-specific ---- */

#if defined(__APPLE__)

/* fcint.h declares FcLocale as locale_t and uses the LC_*_MASK constants.
 * Those live in <xlocale.h>, which macOS <locale.h> does not pull in on every
 * SDK (the 14.0 SDK under -std=c11 does not). config.h is included before
 * fcint.h reaches them, so pull it in here. */
#include <xlocale.h>

/* macOS has no random_r, <sys/vfs.h>, or <sys/statfs.h>; statfs comes from
 * <sys/mount.h> + <sys/param.h> (both declared common above), and its
 * struct statfs carries the filesystem type name in f_fstypename. */
#define HAVE_STRUCT_STATFS_F_FSTYPENAME 1

/* Homebrew (Apple Silicon) layout. Runtime discovery can still be pointed
 * elsewhere via the FONTCONFIG_FILE / FONTCONFIG_PATH environment variables. */
#define CONFIGDIR        "/opt/homebrew/etc/fonts/conf.d"
#define FONTCONFIG_PATH  "/opt/homebrew/etc/fonts"
#define FC_CACHEDIR      "/opt/homebrew/var/cache/fontconfig"
#define FC_TEMPLATEDIR   "/opt/homebrew/share/fontconfig/conf.avail"
#define FC_DEFAULT_FONTS "\t<dir>/System/Library/Fonts</dir>\n\t<dir>/Library/Fonts</dir>\n"
#define FC_FONTPATH      ""

#else /* Linux */

#define HAVE_SYS_STATFS_H 1
#define HAVE_SYS_VFS_H    1
#define HAVE_RANDOM_R     1

/* Debian layout. */
#define CONFIGDIR        "/etc/fonts/conf.d"
#define FONTCONFIG_PATH  "/etc/fonts"
#define FC_CACHEDIR      "/var/cache/fontconfig"
#define FC_TEMPLATEDIR   "/usr/share/fontconfig/conf.avail"
#define FC_DEFAULT_FONTS "\t<dir>/usr/share/fonts</dir>\n\t<dir>/usr/local/share/fonts</dir>\n"
#define FC_FONTPATH      ""

#endif

#endif /* FC_SIMPLE_TEXT_CONFIG_H */
