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

// TODO: For fun; remove this.
uniform float u_time;
uniform vec2 resolution;
vec3 hsl2rgb(vec3 c) {
    vec3 rgb = clamp(abs(mod(c.x * 6.0 + vec3(0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0, 0.0, 1.0);
    return c.z + c.y * (rgb - 0.5) * (1.0 - abs(2.0 * c.z - 1.0));
}

void main() {
    vec4 texel = texture(mask, tex_coords);
    int kind = int(tex_color.a);

    alpha_mask = vec3(texel.a);

    // Plain texture. Color the texture using the input color.
    if (kind == kPlainTexture) {
        // color = tex_color.rgb;

        // TODO: For fun; remove this.
        // https://github.com/tsoding/ded/blob/ea30e9d6ee1c0d52aa11f9386920b884987a6b55/shaders/simple_epic.frag
        vec2 frag_uv = gl_FragCoord.xy / resolution;
        vec3 rainbow = hsl2rgb(vec3((u_time + frag_uv.x + frag_uv.y), 0.5, 0.5));
        color = rainbow;
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
