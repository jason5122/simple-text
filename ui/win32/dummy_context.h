#pragma once

#include <windows.h>

class DummyContext {
public:
    HGLRC m_context;

    void initialize();

private:
    HWND m_hwnd;
};
