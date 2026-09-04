R"(
#version 410

uniform sampler2D atlas;
uniform bool colored;

in vec2 glyph_uv;
flat in vec4 glyph_color;
layout(location = 0, index = 0) out vec4 frag_color;
layout(location = 0, index = 1) out vec4 frag_coverage;

void main() {
    vec4 sample_color = texture(atlas, glyph_uv);
    if (colored) {
        frag_color = sample_color * glyph_color.a;
        frag_coverage = vec4(sample_color.a * glyph_color.a);
    } else {
        frag_color = glyph_color;
        frag_coverage = sample_color * glyph_color.a;
    }
}
)"
