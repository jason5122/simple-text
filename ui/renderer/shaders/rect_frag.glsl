#version 330 core

flat in vec4 rect_color;
flat in vec2 center;
flat in vec2 size;
flat in float corner_radius;

layout(location = 0, index = 0) out vec4 color;
layout(location = 0, index = 1) out vec4 alpha_mask;

uniform vec2 resolution;

float roundedBoxSDF(vec2 pos, vec2 cen, vec2 cor, float radius) {
    vec2 p = pos - cen;
    vec2 q = abs(p) - cor + radius;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - radius;
    // return length(max(abs(pos), cor) - cor) - radius;
}

void main() {
    vec2 lower_left = center - size / 2;
    vec2 cen = gl_FragCoord.xy - lower_left - size / 2;
    float distance = length(max(abs(cen) - size / 2 + corner_radius, 0)) - corner_radius;

    float smoothedAlpha = 1.0 - smoothstep(0.0, 1.0, distance);

    alpha_mask = vec4(1.0, 1.0, 1.0, 1.0);
    color = vec4(rect_color.rgb, smoothedAlpha);
}
