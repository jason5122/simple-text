R"(

#version 330 core

in vec2 tex_coords;
flat in vec4 text_color;

layout(location = 0, index = 0) out vec3 color;
layout(location = 0, index = 1) out vec3 alpha_mask;

uniform sampler2D mask;

const int kPlainTexture = 0;
const int kColoredText = 1;
const int kColoredImage = 2;

void main() {
    vec4 texel = texture(mask, tex_coords);
    int kind = int(text_color.a);

    alpha_mask = vec3(texel.a);

    // Plain texture. We treat alpha as a mask and color the texture using the input color.
    if (kind == kPlainTexture) {
        color = text_color.rgb;
    }
    // Colored texture. The texture already has color.
    else {
        // If colored text, revert alpha premultiplication.
        if (kind == kColoredText && texel.a != 0.0) {
            texel.rgb /= texel.a;
        }
        color = texel.rgb;
    }
}

)"
