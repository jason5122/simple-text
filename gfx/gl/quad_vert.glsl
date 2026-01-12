R"(

#version 330 core

layout(location = 0) in vec2 pos_px;
layout(location = 1) in vec4 color;

uniform vec2 u_viewport_px;
uniform vec2 u_translate_px;

out vec4 v_color;

void main() {
    vec2 pos = pos_px + u_translate_px;
    float x = (pos.x / u_viewport_px.x) * 2.0 - 1.0;
    float y = 1.0 - (pos.y / u_viewport_px.y) * 2.0;
    gl_Position = vec4(x, y, 0.0, 1.0);
    v_color = color;
}

)"
