R"(

#version 330 core

layout(location = 0) in vec2 a_pos_px;
layout(location = 1) in vec4 a_color;

uniform vec2 u_viewport_px;  // (width, height)

out vec4 v_color;

void main() {
    float x = (a_pos_px.x / u_viewport_px.x) * 2.0 - 1.0;
    float y = 1.0 - (a_pos_px.y / u_viewport_px.y) * 2.0;
    gl_Position = vec4(x, y, 0.0, 1.0);
    v_color = a_color;
}

)"
