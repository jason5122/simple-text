#pragma once

#include <memory>

class Renderer {
public:
    static std::unique_ptr<Renderer> create();

    void clear(float r, float g, float b, float a);
    void draw_rect();
    void draw_text();

private:
    Renderer();
};
