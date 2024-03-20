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
    float alpha = 1.0;

    if (corner_radius > 0) {
        vec2 center = gl_FragCoord.xy - rect_center;
        float d = roundedBoxSDF(center, size / 2, corner_radius);
        alpha -= smoothstep(-0.5, 0.5, d);
    }

    vec3 temp = rect_color.rgb;

    if (tab_corner_radius > 0) {
        vec2 pixel_pos = gl_FragCoord.xy;

        vec2 bottom_left = rect_center - size / 2;
        vec2 bottom_right = rect_center + vec2(size.x / 2, -size.y / 2);
        vec2 top_left = rect_center + vec2(-size.x / 2, size.y / 2);
        vec2 top_right = rect_center + size / 2;

        bottom_left += tab_corner_radius;
        bottom_right += vec2(-tab_corner_radius, tab_corner_radius);
        top_left += vec2(tab_corner_radius * 2, -tab_corner_radius);
        top_right += vec2(-tab_corner_radius * 2, -tab_corner_radius);

        if (pixel_pos.x < bottom_left.x) {
            if (pixel_pos.y > bottom_left.y) {
                discard;
            } else {
                vec2 point = rect_center - size / 2;
                point += vec2(0, tab_corner_radius);

                float d = distance(pixel_pos, point) - tab_corner_radius;
                alpha -= smoothstep(-0.5, 0.5, 1.0 - d);
            }
        }
        if (pixel_pos.x > bottom_right.x) {
            if (pixel_pos.y > bottom_right.y) {
                discard;
            } else {
                vec2 point = rect_center + vec2(size.x / 2, -size.y / 2);
                // point += vec2(0, tab_corner_radius);

                float d = distance(pixel_pos, point) - tab_corner_radius;
                // alpha -= smoothstep(-0.5, 0.5, 1.0 - d);

                if (d > 0) {
                    alpha = 0;
                }

                // if (d < -1 || d >= 0) {
                //     alpha = 0;
                // }
            }
        }
        if (pixel_pos.x < top_left.x && pixel_pos.y > top_left.y) {
            float d = distance(pixel_pos, top_left) - tab_corner_radius;
            // alpha -= smoothstep(-0.5, 0.5, d);

            if (-2 < d && d < 0) {
                temp = vec3(1.0, 0.0, 0.0);
            } else if (d > 0) {
                alpha = 0;
            }
        }
        if (pixel_pos.x > top_right.x && pixel_pos.y > top_right.y) {
            float d = distance(pixel_pos, top_right) - tab_corner_radius;
            // alpha -= smoothstep(-0.5, 0.5, d);

            if (-2 < d && d < 0) {
                temp = vec3(1.0, 0.0, 0.0);
            } else if (d > 0) {
                alpha = 0;
            }
        }

        float border_thickness = 2;
        float border_left = rect_center.x - size.x / 2 + tab_corner_radius + border_thickness;
        float border_right = rect_center.x + size.x / 2 - tab_corner_radius - border_thickness;
        float border_top = rect_center.y + size.y / 2 - border_thickness;
        float border_bottom = rect_center.y - size.y / 2 + border_thickness;
        if (pixel_pos.x < border_left) {
            temp = vec3(1.0, 0.0, 0.0);
        }
        if (pixel_pos.x > border_right) {
            temp = vec3(1.0, 0.0, 0.0);
        }
        if (pixel_pos.y > border_top) {
            temp = vec3(1.0, 0.0, 0.0);
        }
        if (pixel_pos.y < border_bottom) {
            temp = vec3(1.0, 0.0, 0.0);
        }
    }

    alpha_mask = vec4(1.0);
    color = vec4(temp, alpha);
}
