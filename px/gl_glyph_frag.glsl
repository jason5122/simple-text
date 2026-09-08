R"(
uniform sampler2D atlas;
uniform bool colored;
uniform bool alternate;

in vec2 glyph_uv;
flat in vec4 glyph_color;
layout(location = 0, index = 0) out vec4 frag_color;
layout(location = 0, index = 1) out vec4 frag_coverage;

void main() {
    vec4 sample_color = texture(atlas, glyph_uv);
    if (alternate && !colored) {
        // The alternate cache stores monochrome glyphs at reversed polarity: black ink on an
        // opaque white background. Recover coverage the same way the software compositor does
        // (composite_glyph_scanline), leaving the rasterized alpha alone.
        sample_color.rgb = vec3(1.0) - sample_color.rgb;
    }
    if (colored) {
        frag_color = sample_color * glyph_color.a;
        frag_coverage = vec4(sample_color.a * glyph_color.a);
    } else {
        frag_color = glyph_color;
        frag_coverage = sample_color * glyph_color.a;
    }
}
)"
