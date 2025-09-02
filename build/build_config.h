#pragma once

// These macros un-mangle the names of the build flags in a way that looks natural, and gives
// errors if the flag is not defined. Normally in the preprocessor it's easy to make mistakes that
// interpret "you haven't done the setup to know what the flag is" as "flag is off". Normally you
// would include the generated header rather than include this file directly.
#define BUILDFLAG(flag) (BUILDFLAG_CAT(BUILDFLAG_INTERNAL_, flag)())

// This dance of two macros does a concatenation of two preprocessor args using ## doubly
// indirectly because using ## directly prevents macros in that parameter from being expanded.
#define BUILDFLAG_CAT_INDIRECT(a, b) a##b
#define BUILDFLAG_CAT(a, b) BUILDFLAG_CAT_INDIRECT(a, b)

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
