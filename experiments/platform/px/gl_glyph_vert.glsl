R"(
#version 410

uniform samplerBuffer instances;
uniform vec2 viewport;
uniform int instance_offset;
uniform float texture_size;

out vec2 glyph_uv;
out vec2 glyph_position;
flat out vec4 glyph_color;
flat out float glyph_colored;
flat out float glyph_alternate;
flat out vec4 glyph_clip;

void main() {
    int instance_index = gl_InstanceID + instance_offset;
    vec4 dst = texelFetch(instances, instance_index * 5 + 0);
    vec4 texture_source = texelFetch(instances, instance_index * 5 + 1);
    glyph_color = texelFetch(instances, instance_index * 5 + 2);
    vec4 glyph_flags = texelFetch(instances, instance_index * 5 + 3);
    glyph_colored = glyph_flags.x;
    glyph_alternate = glyph_flags.y;
    glyph_clip = texelFetch(instances, instance_index * 5 + 4);

    vec2 corner;
    corner.x = (gl_VertexID == 1 || gl_VertexID == 3) ? 1.0 : 0.0;
    corner.y = (gl_VertexID >= 2) ? 1.0 : 0.0;
    vec2 position = dst.xy + corner * dst.zw;
    glyph_position = position;
    vec2 ndc = position / viewport * 2.0 - 1.0;
    gl_Position = vec4(ndc.x, -ndc.y, 0.0, 1.0);
    vec2 texture_position = texture_source.xy + corner * texture_source.zw;
    glyph_uv = texture_position / texture_size;
}
)"
