R"(

#version 330 core

in vec2 tex_coords;
flat in vec4 text_color;

layout(location = 0, index = 0) out vec4 color;
layout(location = 0, index = 1) out vec4 alpha_mask;

uniform sampler2D mask;

void main() {
    vec4 texel = texture(mask, tex_coords);

    int colored = int(text_color.a);
    if (colored == 1) {
        // Revert alpha premultiplication.
        if (texel.a != 0.0) {
            texel.rgb /= texel.a;
        }

        alpha_mask = vec4(texel.a);
        color = vec4(texel.rgb, 1.0);
    } else {
        alpha_mask = vec4(texel.rgb, texel.r);
        color = vec4(text_color.rgb, 1.0);
    }
}

)"
