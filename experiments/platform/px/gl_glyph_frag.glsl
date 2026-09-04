R"(
#version 410

uniform sampler2D atlas;

in vec2 glyph_uv;
in vec2 glyph_position;
flat in vec4 glyph_color;
flat in float glyph_colored;
flat in float glyph_alternate;
flat in vec4 glyph_clip;
layout(location = 0, index = 0) out vec4 frag_color;
layout(location = 0, index = 1) out vec4 frag_coverage;

void main() {
    if (glyph_position.x < glyph_clip.x || glyph_position.x >= glyph_clip.z ||
        glyph_position.y < glyph_clip.y || glyph_position.y >= glyph_clip.w) {
        discard;
    }
    vec4 sample_color = texture(atlas, glyph_uv);
    if (glyph_colored > 0.5) {
        frag_color = sample_color * glyph_color.a;
        frag_coverage = vec4(sample_color.a * glyph_color.a);
    } else {
        frag_color = glyph_color;
        if (glyph_alternate > 0.5) {
            sample_color.rgb = vec3(1.0) - sample_color.rgb;
        }
        frag_coverage = sample_color * glyph_color.a;
    }
}
)"
