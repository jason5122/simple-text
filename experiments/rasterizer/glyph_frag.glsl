R"(

#version 330 core

in vec2 v_uv;
flat in vec4 v_color;
flat in float v_flags;

out vec4 o_color;

uniform sampler2D u_tex;

void main() {
    vec4 texel = texture(u_tex, v_uv);
    if (v_flags > 1.5) {
        // Atlas debug view: composite the atlas over a gray field so its bounds show on the white
        // window and color glyphs keep their color.
        vec3 bg = vec3(0.85);
        o_color = vec4(texel.rgb + bg * (1.0 - texel.a), 1.0);
    } else if (v_flags > 0.5) {
        // Color glyph (emoji): premultiplied RGBA, drawn as-is.
        o_color = texel;
    } else {
        // Mono glyph: coverage is in alpha (rgb is 0), so it's a mask -- tint it with v_color.
        // Output stays premultiplied for GL_ONE / GL_ONE_MINUS_SRC_ALPHA.
        o_color = vec4(v_color.rgb * texel.a, texel.a);
    }
}

)"
