#version 330 core

in vec2 tex_coords;

layout(location = 0, index = 0) out vec4 color;
layout(location = 0, index = 1) out vec4 alpha_mask;

uniform sampler2D mask;

void main() {
    vec4 texel = texture(mask, tex_coords);

    if (texel.a == 0) {
        discard;
    }

    alpha_mask = vec4(1.0);
    color = vec4(texel.rgb, texel.a);

    // DEBUG: This colors the entire background of texture for easy locating.
    // color = mix(color, vec4(1.0, 0.0, 0.0, 1.0), 0.5);

    // color = mix(color, vec4(0.494, 0.494, 0.494, 1.0), texel.a);
}
