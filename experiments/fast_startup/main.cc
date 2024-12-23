#include "build/build_config.h"
#include "experiments/fast_startup/fast_startup_app.h"

namespace {

int FastStartupAppMain(int argc, char* argv[]) {
    // TODO: Return proper status codes.
    FastStartupApp fast_startup_app;
    fast_startup_app.run();
    return 0;
}

}  // namespace

#if BUILDFLAG(IS_WIN)
#include <ole2.h>
#include <shellscalingapi.h>

INT WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, INT nCmdShow) {
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    int status_code = FastStartupAppMain(0, nullptr);
    CoUninitialize();
    return status_code;
}
#else
int main(int argc, char* argv[]) {
    return FastStartupAppMain(argc, argv);
}
#endif
