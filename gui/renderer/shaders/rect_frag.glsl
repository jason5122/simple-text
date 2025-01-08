R"(

#version 330 core

flat in vec4 rect_color;
flat in vec2 rect_center;
flat in vec2 size;
// <corner_radius, tab_corner_radius, left_shadow, right_shadow>
flat in vec4 extra;

out vec4 out_color;

// https://www.reddit.com/r/opengl/comments/sbrykq/comment/hu4dqj9/?utm_source=share&utm_medium=web2x&context=3
float roundedBoxSDF(vec2 center, vec2 size, float radius) {
    return length(max(abs(center) - size + radius, 0.0)) - radius;
}

void main() {
    vec2 coord = gl_FragCoord.xy;
    float computed_alpha = rect_color.a;
    float corner_radius = extra.x;
    float tab_corner_radius = extra.y;
    float left_shadow = extra.z;
    float right_shadow = extra.w;

    if (corner_radius > 0) {
        vec2 center = coord - rect_center;
        float d = roundedBoxSDF(center, size / 2, corner_radius);
        computed_alpha -= smoothstep(-0.5, 0.5, d);
    }
    if (tab_corner_radius > 0) {
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
        if (coord.x < curve_top_left.x && coord.y > curve_top_left.y) {
            float d = distance(coord, curve_top_left) - tab_corner_radius;
            computed_alpha = 1.0 - smoothstep(-0.5, 0.5, d);
        }
        if (coord.x > curve_top_right.x && coord.y > curve_top_right.y) {
            float d = distance(coord, curve_top_right) - tab_corner_radius;
            computed_alpha = 1.0 - smoothstep(-0.5, 0.5, d);
        }

        vec2 curve_bottom_left_outwards = bottom_left + vec2(-tab_corner_radius, tab_corner_radius);
        vec2 curve_bottom_right_outwards = bottom_right + vec2(tab_corner_radius, tab_corner_radius);
        if (coord.x < curve_bottom_left_outwards.x + tab_corner_radius) {
            computed_alpha = 0.0;
            if (coord.y < curve_bottom_left_outwards.y) {
                float d = distance(coord, curve_bottom_left_outwards) - tab_corner_radius;
                computed_alpha = smoothstep(-0.5, 0.5, d);
            }
        }
        if (coord.x > curve_bottom_right_outwards.x - tab_corner_radius) {
            computed_alpha = 0.0;
            if (coord.y < curve_bottom_right_outwards.y) {
                float d = distance(coord, curve_bottom_right_outwards) - tab_corner_radius;
                computed_alpha = smoothstep(-0.5, 0.5, d);
            }
        }
    }
    if (left_shadow > 0) {
        float left_edge = rect_center.x - size.x / 2;
        float d = max(distance(coord.x, left_edge), 0.0);
        computed_alpha = max(1.0 - 0.1 * d, 0.0);
    }
    if (right_shadow > 0) {
        float right_edge = rect_center.x + size.x / 2;
        float d = max(distance(coord.x, right_edge), 0.0);
        computed_alpha = max(1.0 - 0.1 * d, 0.0);
    }

    out_color = vec4(rect_color.rgb, computed_alpha);
}

)"
