#include "build/build_config.h"
#include "experiments/fast_startup/app.h"

namespace {

int EditorMain(int argc, char* argv[]) {
    // Disable stdout buffering.
    std::setbuf(stdout, nullptr);

    // TODO: Return proper status codes.
    gui::FastStartupApp fast_startup_app;
    fast_startup_app.run();
    return 0;
}

}  // namespace

#if BUILDFLAG(IS_WIN)
#include <ole2.h>
#include <shellscalingapi.h>

INT WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, INT nCmdShow) {
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    int status_code = EditorMain(0, nullptr);
    CoUninitialize();
    return status_code;
}
#else
int main(int argc, char* argv[]) { return EditorMain(argc, argv); }
#endif
