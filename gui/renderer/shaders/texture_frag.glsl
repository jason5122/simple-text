R"(

#version 330 core

in vec2 tex_coords;
// The `kind` flag is packed along with texture color.
flat in vec4 tex_color;

// TODO: See if we are required to output a `vec4` here.
layout(location = 0, index = 0) out vec3 color;
layout(location = 0, index = 1) out vec3 alpha_mask;

uniform sampler2D mask;

const int kPlainTexture = 0;
const int kColoredText = 1;
const int kColoredImage = 2;

void main() {
    const int kDebug = 0;
    if (kDebug == 1) {
        color = vec3(1.0);
        alpha_mask = vec3(1.0);
        return;
    }

    vec4 texel = texture(mask, tex_coords);
    int kind = int(tex_color.a);

    alpha_mask = vec3(texel.a);

    // Plain texture. Color the texture using the input color.
    if (kind == kPlainTexture) {
        color = tex_color.rgb;
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
