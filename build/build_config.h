#pragma once

#include "build/buildflag.h"

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
