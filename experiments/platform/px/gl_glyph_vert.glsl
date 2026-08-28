R"(
#version 150

uniform samplerBuffer instances;
uniform vec2 viewport;
uniform int instance_offset;
uniform float texture_size;

out vec2 glyph_uv;
flat out vec4 glyph_color;
flat out float glyph_colored;

void main() {
    int instance_index = gl_InstanceID + instance_offset;
    vec4 dst = texelFetch(instances, instance_index * 4 + 0);
    vec4 uv = texelFetch(instances, instance_index * 4 + 1);
    glyph_color = texelFetch(instances, instance_index * 4 + 2);
    glyph_colored = texelFetch(instances, instance_index * 4 + 3).x;

    vec2 corner;
    corner.x = (gl_VertexID == 1 || gl_VertexID == 3) ? 1.0 : 0.0;
    corner.y = (gl_VertexID >= 2) ? 1.0 : 0.0;
    vec2 position = dst.xy + corner * dst.zw;
    vec2 ndc = position / viewport * 2.0 - 1.0;
    gl_Position = vec4(ndc.x, -ndc.y, 0.0, 1.0);
    vec2 texture_position = uv.xy + corner * uv.zw;
    glyph_uv = texture_position / texture_size;
}
)"
