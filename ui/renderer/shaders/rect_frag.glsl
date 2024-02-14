#version 330 core

flat in vec4 rect_color;
flat in vec2 rect_center;
flat in vec2 size;
flat in float corner_radius;

layout(location = 0, index = 0) out vec4 color;
layout(location = 0, index = 1) out vec4 alpha_mask;

uniform vec2 resolution;

float roundedBoxSDF(vec2 center, vec2 size, float radius) {
    return length(max(abs(center) - size + radius, 0.0)) - radius;
}

void main() {
    vec2 lower_left = rect_center - size / 2;
    vec2 center = gl_FragCoord.xy - lower_left - size / 2;
    float distance = roundedBoxSDF(center, size / 2, corner_radius);
    float smoothedAlpha = 1.0 - smoothstep(0.0, 1.0, distance);

    alpha_mask = vec4(1.0, 1.0, 1.0, 1.0);
    color = vec4(rect_color.rgb, smoothedAlpha);
}
