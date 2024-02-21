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

    // vec2 center = gl_FragCoord.xy - rect_center;
    // if (corner_radius > 0) {
    //     float distance = roundedBoxSDF(center, size / 2, corner_radius);
    //     alpha -= smoothstep(-0.5, 0.5, distance);
    // }

    vec2 pixel_pos = gl_FragCoord.xy;

    vec2 bottom_left = rect_center - size / 2;
    vec2 bottom_right = rect_center + vec2(size.x / 2, -size.y / 2);
    vec2 top_left = rect_center + vec2(-size.x / 2, size.y / 2);
    vec2 top_right = rect_center + size / 2;

    bottom_left += corner_radius;
    bottom_right += vec2(-corner_radius, corner_radius);
    top_left += vec2(corner_radius, -corner_radius);
    top_right -= corner_radius;

    if (pixel_pos.x < bottom_left.x && pixel_pos.y < bottom_left.y) {
        if (distance(pixel_pos, bottom_left) > corner_radius) {
            discard;
        }
    }

    vec2 point = rect_center + vec2(size.x / 2, -size.y / 2);
    point += vec2(-corner_radius, corner_radius);
    if (pixel_pos.x > point.x && pixel_pos.y > point.y) {
        discard;
    }
    vec2 point2 = rect_center + vec2(size.x / 2, -size.y / 2);
    point2 += vec2(0, corner_radius);
    if (pixel_pos.x > point.x && pixel_pos.y < point.y) {
        if (distance(pixel_pos, point2) < corner_radius) {
            discard;
        }
    }

    alpha_mask = vec4(1.0);
    color = vec4(rect_color.rgb, alpha);
}
