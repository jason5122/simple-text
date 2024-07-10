R"(

#version 330 core

flat in vec4 rect_color;
flat in vec2 rect_center;
flat in vec2 size;
flat in float corner_radius;      // TODO: Consider making this a uniform.
flat in float tab_corner_radius;  // TODO: Remove the `tab_corner_radius` experiment.

layout(location = 0, index = 0) out vec4 color;
layout(location = 0, index = 1) out vec4 alpha_mask;

// https://www.reddit.com/r/opengl/comments/sbrykq/comment/hu4dqj9/?utm_source=share&utm_medium=web2x&context=3
float roundedBoxSDF(vec2 center, vec2 size, float radius) {
    return length(max(abs(center) - size + radius, 0.0)) - radius;
}

void main() {
    float computed_alpha = 1.0;

    if (corner_radius > 0) {
        vec2 center = gl_FragCoord.xy - rect_center;
        float d = roundedBoxSDF(center, size / 2, corner_radius);
        computed_alpha -= smoothstep(-0.5, 0.5, d);
    }
    if (tab_corner_radius > 0) {
        vec2 pixel_pos = gl_FragCoord.xy;

        vec2 bottom_left = rect_center - size / 2;
        vec2 bottom_right = rect_center + vec2(size.x / 2, -size.y / 2);
        vec2 top_left = rect_center + vec2(-size.x / 2, size.y / 2);
        vec2 top_right = rect_center + size / 2;

        bottom_left.x += tab_corner_radius;
        bottom_right.x -= tab_corner_radius;
        top_left.x += tab_corner_radius;
        top_right.x -= tab_corner_radius;

        vec2 curve_top_left = top_left + vec2(tab_corner_radius, -tab_corner_radius);
        vec2 curve_top_right = top_right + vec2(-tab_corner_radius, -tab_corner_radius);
        if (pixel_pos.x < curve_top_left.x && pixel_pos.y > curve_top_left.y) {
            float d = distance(pixel_pos, curve_top_left) - tab_corner_radius;
            computed_alpha = 1.0 - smoothstep(-0.5, 0.5, d);
        }
        if (pixel_pos.x > curve_top_right.x && pixel_pos.y > curve_top_right.y) {
            float d = distance(pixel_pos, curve_top_right) - tab_corner_radius;
            computed_alpha = 1.0 - smoothstep(-0.5, 0.5, d);
        }

        vec2 curve_bottom_left_outwards = bottom_left + vec2(-tab_corner_radius, tab_corner_radius);
        vec2 curve_bottom_right_outwards = bottom_right + vec2(tab_corner_radius, tab_corner_radius);
        if (pixel_pos.x < curve_bottom_left_outwards.x + tab_corner_radius) {
            computed_alpha = 0.0;
            if (pixel_pos.y < curve_bottom_left_outwards.y) {
                float d = distance(pixel_pos, curve_bottom_left_outwards) - tab_corner_radius;
                computed_alpha = smoothstep(-0.5, 0.5, d);
            }
        }
        if (pixel_pos.x > curve_bottom_right_outwards.x - tab_corner_radius) {
            computed_alpha = 0.0;
            if (pixel_pos.y < curve_bottom_right_outwards.y) {
                float d = distance(pixel_pos, curve_bottom_right_outwards) - tab_corner_radius;
                computed_alpha = smoothstep(-0.5, 0.5, d);
            }
        }
    }

    alpha_mask = vec4(1.0);
    color = vec4(rect_color.rgb, computed_alpha);
}

)"
