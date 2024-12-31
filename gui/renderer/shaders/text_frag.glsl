R"(

#version 330 core

in vec2 tex_coords;
flat in vec4 text_color;

layout(location = 0, index = 0) out vec4 color;
layout(location = 0, index = 1) out vec4 alpha_mask;

uniform sampler2D mask;

void main() {
    vec4 texel = texture(mask, tex_coords);

    int mode = int(text_color.a);
    // Plain text.
    if (mode == 0) {
        color = vec4(text_color.rgb, 1.0);
        alpha_mask = vec4(texel.rgb, texel.r);
    }
    // Colored text (e.g., emojis).
    else if (mode == 1) {
        // Revert alpha premultiplication.
        if (texel.a != 0.0) {
            texel.rgb /= texel.a;
        }

        color = vec4(texel.rgb, 1.0);
        alpha_mask = vec4(texel.a);
    }
    // Plain image.
    else if (mode == 2) {
        color = vec4(text_color.rgb, texel.a);
        alpha_mask = vec4(texel.a);
    }
    // Colored image.
    else if (mode == 3) {
        color = texel;
        alpha_mask = vec4(texel.a);
    }
}

)"
