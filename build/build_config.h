#pragma once

// These macros un-mangle the names of the build flags in a way that looks
// natural, and gives errors if the flag is not defined. Normally in the
// preprocessor it's easy to make mistakes that interpret "you haven't done
// the setup to know what the flag is" as "flag is off". Normally you would
// include the generated header rather than include this file directly.
//
// This is for use with generated headers. See build/buildflag_header.gni.

// This dance of two macros does a concatenation of two preprocessor args using
// ## doubly indirectly because using ## directly prevents macros in that
// parameter from being expanded.
#define BUILDFLAG_CAT_INDIRECT(a, b) a##b
#define BUILDFLAG_CAT(a, b) BUILDFLAG_CAT_INDIRECT(a, b)

// Accessor for build flags.
//
// To test for a value, if the build file specifies:
//
//   ENABLE_FOO=true
//
// Then you would check at build-time in source code with:
//
//   #include "foo_flags.h"  // The header the build file specified.
//
//   #if BUILDFLAG(ENABLE_FOO)
//     ...
//   #endif
//
// There will no #define called ENABLE_FOO so if you accidentally test for
// whether that is defined, it will always be negative. You can also use
// the value in expressions:
//
//   const char kSpamServerName[] = BUILDFLAG(SPAM_SERVER_NAME);
//
// Because the flag is accessed as a preprocessor macro with (), an error
// will be thrown if the proper header defining the internal flag value has
// not been included.
#define BUILDFLAG(flag) (BUILDFLAG_CAT(BUILDFLAG_INTERNAL_, flag)())

#if defined(__APPLE__)
#define BUILDFLAG_INTERNAL_IS_MAC() (1)
#else
#define BUILDFLAG_INTERNAL_IS_MAC() (0)
#endif

#if defined(__linux__)
#define BUILDFLAG_INTERNAL_IS_LINUX() (1)
#else
#define BUILDFLAG_INTERNAL_IS_LINUX() (0)
#endif

#if defined(_WIN32)
#define BUILDFLAG_INTERNAL_IS_WIN() (1)
#else
#define BUILDFLAG_INTERNAL_IS_WIN() (0)
#endif

#if defined(__APPLE__) || defined(__linux__)
#define BUILDFLAG_INTERNAL_IS_POSIX() (1)
#else
#define BUILDFLAG_INTERNAL_IS_POSIX() (0)
#endif
