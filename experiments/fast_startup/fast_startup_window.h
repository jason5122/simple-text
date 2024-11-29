#pragma once

#include "app/window.h"

class FastStartupApp;

class FastStartupWindow : public app::Window {
public:
    FastStartupWindow(FastStartupApp& parent, int width, int height, int wid);

    void onDraw(const app::Size& size) override;
};
