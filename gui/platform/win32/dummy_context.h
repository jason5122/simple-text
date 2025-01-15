#pragma once

#include <windows.h>

namespace gui {

class DummyContext {
public:
    HGLRC m_context;

    void initialize();

private:
    HWND m_hwnd;
};

}  // namespace gui
