R"(
uniform sampler2D atlas;
uniform bool colored;
uniform bool alternate;
#if defined(PX_LINUX_EXACT_COMPOSITE)
uniform sampler2D destination;
#endif

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
#if defined(PX_LINUX_EXACT_COMPOSITE)
    uvec4 sample_bytes = uvec4(round(clamp(sample_color, 0.0, 1.0) * 255.0));
    uvec4 tint_bytes = uvec4(round(clamp(glyph_color, 0.0, 1.0) * 255.0));
    uvec4 destination_bytes =
        uvec4(round(clamp(texelFetch(destination, ivec2(gl_FragCoord.xy), 0), 0.0, 1.0) *
                    255.0));
    uvec4 result;
    if (colored) {
        uint source_alpha = sample_bytes.a * tint_bytes.a / 255u;
        for (int channel = 0; channel < 4; ++channel) {
            uint source = sample_bytes[channel] * tint_bytes.a / 255u;
            result[channel] =
                min(source + destination_bytes[channel] * (255u - source_alpha) / 255u, 255u);
        }
    } else {
        uvec4 coverage = sample_bytes * tint_bytes.a / 255u;
        for (int channel = 0; channel < 3; ++channel) {
            uint source = tint_bytes[channel] * coverage[channel] / 255u;
            uint product = destination_bytes[channel] * (255u - coverage[channel]);
            uint attenuated = min((product + (product >> 8u) + 2u) >> 8u, 255u);
            result[channel] = min(source + attenuated, 255u);
        }
        result.a = min(coverage.a + destination_bytes.a * (255u - coverage.a) / 255u, 255u);
    }
    frag_color = vec4(result) / 255.0;
    frag_coverage = vec4(0.0);
#else
    if (colored) {
        frag_color = sample_color * glyph_color.a;
        frag_coverage = vec4(sample_color.a * glyph_color.a);
    } else {
        frag_color = glyph_color;
        frag_coverage = sample_color * glyph_color.a;
    }
#endif
}
)"
