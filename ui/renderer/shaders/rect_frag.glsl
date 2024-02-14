#version 330 core

flat in vec4 rect_color;
flat in vec2 rect_center;
flat in vec2 size;
flat in float corner_radius;

layout(location = 0, index = 0) out vec4 color;
layout(location = 0, index = 1) out vec4 alpha_mask;

// https://www.reddit.com/r/opengl/comments/sbrykq/comment/hu4dqj9/?utm_source=share&utm_medium=web2x&context=3
float roundedBoxSDF(vec2 center, vec2 size, float radius) {
    return length(max(abs(center) - size + radius, 0.0)) - radius;
}

void main() {
    float alpha = 1.0;

    if (corner_radius > 0) {
        vec2 center = gl_FragCoord.xy - rect_center;
        float distance = roundedBoxSDF(center, size / 2, corner_radius);
        alpha -= smoothstep(-0.5, 0.5, distance);
    }

    alpha_mask = vec4(1.0);
    color = vec4(rect_color.rgb, alpha);
}
