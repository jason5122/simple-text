R"(

#version 330 core

in vec2 v_uv;
in vec4 v_color;
out vec4 o_color;

uniform sampler2D u_tex;

void main() { o_color = texture(u_tex, v_uv) * v_color; }

)"
