R"(

#version 330 core

in vec2 v_uv;
flat in vec4 v_color;
flat in float v_flags;

// Dual-source blending: o_color is the premultiplied colour, o_cover the per-channel coverage the
// blender subtracts from the destination. It is what lets a mono glyph carry a different coverage
// in R, G and B -- which is what DirectWrite's ClearType produces, and what Sublime's own shader
// consumes. On a platform whose rasteriser is grayscale the three channels are simply equal, and
// the result is identical to blending against a single alpha.
layout(location = 0, index = 0) out vec4 o_color;
layout(location = 0, index = 1) out vec4 o_cover;

uniform sampler2D u_tex;

void main() {
    vec4 texel = texture(u_tex, v_uv);
    if (v_flags > 1.5) {
        // Atlas debug view: composite the atlas over a gray field so its bounds show on the white
        // window and color glyphs keep their color.
        vec3 bg = vec3(0.85);
        o_color = vec4(texel.rgb + bg * (1.0 - texel.a), 1.0);
        o_cover = vec4(1.0);
    } else if (v_flags > 0.5) {
        // Color glyph (emoji): premultiplied RGBA, drawn as-is, so its coverage is a flat alpha.
        o_color = texel;
        o_cover = vec4(texel.a);
    } else {
        // Mono glyph: rgb holds the coverage of each subpixel, so tint and blend per channel.
        o_color = vec4(v_color.rgb * texel.rgb, texel.a);
        o_cover = vec4(texel.rgb, texel.a);
    }
}

)"
