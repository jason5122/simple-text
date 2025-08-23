#pragma once

#include "build/build_config.h"

#if BUILDFLAG(IS_POSIX)
#include "experiments/gui_api_redesign/events/keycodes/keyboard_codes_posix.h"
#elif BUILDFLAG(IS_WIN)
#include "experiments/gui_api_redesign/events/keycodes/keyboard_codes_win.h"
#endif
