R"(
#version 150

uniform sampler2D atlas;

in vec2 glyph_uv;
flat in vec4 glyph_color;
flat in float glyph_colored;
out vec4 frag_color;
out vec4 frag_coverage;

void main() {
    vec4 sample_color = texture(atlas, glyph_uv);
    if (glyph_colored > 0.5) {
        frag_color = sample_color * glyph_color.a;
        frag_coverage = vec4(sample_color.a * glyph_color.a);
    } else {
        vec4 coverage = vec4(sample_color.rgb, sample_color.a) * glyph_color.a;
        frag_color = glyph_color;
        frag_coverage = coverage;
    }
}
)"
