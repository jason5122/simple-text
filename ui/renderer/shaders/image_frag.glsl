#version 330 core

in vec2 tex_coords;

layout(location = 0, index = 0) out vec4 color;
layout(location = 0, index = 1) out vec4 alpha_mask;

uniform sampler2D mask;

void main() {
    vec4 texel = texture(mask, tex_coords);

    alpha_mask = vec4(1.0);
    // color = vec4(0.455, 0.455, 0.455, texel.a);
    color = vec4(0.62, 0.62, 0.62, texel.a);
}
