#pragma once

#include "base/apple/owned_nsevent.h"
#include "build/build_config.h"

namespace ui {

#if BUILDFLAG(IS_MAC)
using PlatformEvent = base::apple::OwnedNSEvent;
#elif BUILDFLAG(IS_LINUX)
using PlatformEvent = ui::Event*;
#elif BUILDFLAG(IS_WIN)
// From Chromium //base/win/windows_types.h.
struct CHROME_MSG {
    HWND hwnd;
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
    DWORD time;
    CHROME_POINT pt;
};
using PlatformEvent = CHROME_MSG;
#endif

}  // namespace ui
