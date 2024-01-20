#version 330 core

in vec2 tex_coords;

layout(location = 0, index = 0) out vec4 out_color;
layout(location = 0, index = 1) out vec4 out_alpha_mask;

uniform sampler2D mask;

void main() {
    vec3 black = vec3(51, 51, 51) / 255.0;
    vec3 yellow = vec3(249, 174, 88) / 255.0;
    vec3 blue = vec3(102, 153, 204) / 255.0;

    vec3 text_color = texture(mask, tex_coords).rgb;
    out_alpha_mask = vec4(text_color, text_color.r);
    out_color = vec4(yellow, 1.0);
}
