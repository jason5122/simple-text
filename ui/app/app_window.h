#pragma once

#include <cstdint>
#include <string>

class AppWindow {
public:
    enum KeyModifierFlags : uint32_t {
        Shift = 1 << 1,
        Control = 1 << 2,
        Alt = 1 << 3,
        Super = 1 << 4,
    };

    virtual void onOpenGLActivate(int width, int height) = 0;
    virtual void onDraw() = 0;
    virtual void onResize(int width, int height) = 0;
    virtual void onScroll(float dx, float dy) = 0;
    virtual void onLeftMouseDown(float mouse_x, float mouse_y) = 0;
    virtual void onLeftMouseDrag(float mouse_x, float mouse_y) = 0;
    virtual void onKeyDown(std::string chars, KeyModifierFlags modifiers) = 0;
};

inline constexpr AppWindow::KeyModifierFlags operator|(AppWindow::KeyModifierFlags a,
                                                       AppWindow::KeyModifierFlags b) {
    uint32_t a_value = static_cast<uint32_t>(a);
    uint32_t b_value = static_cast<uint32_t>(b);
    return static_cast<AppWindow::KeyModifierFlags>(a_value | b_value);
}

inline constexpr AppWindow::KeyModifierFlags operator&(AppWindow::KeyModifierFlags a,
                                                       AppWindow::KeyModifierFlags b) {
    uint32_t a_value = static_cast<uint32_t>(a);
    uint32_t b_value = static_cast<uint32_t>(b);
    return static_cast<AppWindow::KeyModifierFlags>(a_value & b_value);
}

inline constexpr AppWindow::KeyModifierFlags operator|=(AppWindow::KeyModifierFlags& a,
                                                        AppWindow::KeyModifierFlags b) {
    return a = a | b;
}
