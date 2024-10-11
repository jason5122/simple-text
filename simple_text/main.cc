#include "build/build_config.h"
#include "simple_text/editor_app.h"

namespace {

int SimpleTextMain(int argc, char* argv[]) {
    // TODO: Return proper status codes.
    EditorApp editor_app;
    editor_app.run();
    return 0;
}

}

#if BUILDFLAG(IS_WIN)
#include <ole2.h>
#include <shellscalingapi.h>

INT WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, INT nCmdShow) {
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    int status_code = SimpleTextMain(0, nullptr);
    CoUninitialize();
    return status_code;
}
#else
int main(int argc, char* argv[]) {
    return SimpleTextMain(argc, argv);
}
#endif
