/*
 * Hand-maintained expat configuration for the vendored build.
 *
 * expat's sources #include "expat_config.h" unconditionally; upstream
 * generates it via CMake/autoconf. We don't, so this is checked in. It targets
 * POSIX (macOS + Linux) on 64-bit little-endian; a Windows build would use
 * expat's own winconfig.h instead. See README.md.
 */

#ifndef EXPAT_CONFIG_H
#define EXPAT_CONFIG_H 1

/* Both target arches (x64/arm64) are little-endian. */
#define BYTEORDER 1234

/*
 * Entropy source for hash-collision salting: getentropy(). Available on glibc
 * >= 2.25 (the Debian bullseye sysroot is 2.31) and macOS >= 10.12, so one
 * backend covers both platforms. random_getentropy.c picks the right header.
 */
#define HAVE_GETENTROPY

/* Standard POSIX headers/functions (macOS + Linux). */
#define HAVE_DLFCN_H
#define HAVE_FCNTL_H
#define HAVE_GETPAGESIZE
#define HAVE_INTTYPES_H
#define HAVE_MMAP
#define HAVE_STDINT_H
#define HAVE_STDLIB_H
#define HAVE_STRINGS_H
#define HAVE_STRING_H
#define HAVE_SYS_STAT_H
#define HAVE_SYS_TYPES_H
#define HAVE_UNISTD_H
#define STDC_HEADERS 1

/* Parser feature set (expat's standard defaults). */
#define XML_CONTEXT_BYTES 1024
#define XML_DTD
#define XML_GE 1
#define XML_NS

#define PACKAGE_NAME "expat"
#define PACKAGE_STRING "expat 2.8.2"
#define PACKAGE_TARNAME "expat"
#define PACKAGE_VERSION "2.8.2"
#define PACKAGE_BUGREPORT "https://github.com/libexpat/libexpat/issues"

#endif /* EXPAT_CONFIG_H */
