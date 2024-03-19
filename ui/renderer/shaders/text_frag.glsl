#version 330 core

in vec2 tex_coords;
flat in vec4 text_color;
flat in vec4 bg_color;
flat in vec2 bg_center;
flat in vec2 bg_size;

layout(location = 0, index = 0) out vec4 color;
layout(location = 0, index = 1) out vec4 alpha_mask;

uniform sampler2D mask;
uniform int rendering_pass;

// https://www.reddit.com/r/opengl/comments/sbrykq/comment/hu4dqj9/?utm_source=share&utm_medium=web2x&context=3
float roundedBoxSDF(vec2 center, vec2 size, float radius) {
    return length(max(abs(center) - size + radius, 0.0)) - radius;
}

void main() {
    if (rendering_pass == 0) {
        // if (bg_color.a == 0.0) discard;

        float alpha = 1.0;
        vec3 temp = bg_color.rgb;
        float tab_corner_radius = 6;

        if (tab_corner_radius > 0) {
            vec2 center = gl_FragCoord.xy - bg_center;
            float d = roundedBoxSDF(center, bg_size / 2, tab_corner_radius);
            if (d < 0) {
                alpha = 1.0;
            } else {
                alpha = 0.0;
            }
        }

        // if (tab_corner_radius > 0) {
        //     vec2 pixel_pos = gl_FragCoord.xy;

        //     vec2 bottom_left = bg_center - bg_size / 2;
        //     vec2 bottom_right = bg_center + vec2(bg_size.x / 2, -bg_size.y / 2);
        //     vec2 top_left = bg_center + vec2(-bg_size.x / 2, bg_size.y / 2);
        //     vec2 top_right = bg_center + bg_size / 2;

        //     bottom_left += tab_corner_radius;
        //     bottom_right += vec2(-tab_corner_radius, tab_corner_radius);
        //     top_left += vec2(tab_corner_radius, -tab_corner_radius);
        //     top_right += vec2(-tab_corner_radius, -tab_corner_radius);

        //     // if (pixel_pos.x < bottom_left.x) {
        //     //     if (pixel_pos.y > bottom_left.y) {
        //     //         // discard;
        //     //     } else {
        //     //         vec2 point = bg_center - bg_size / 2;
        //     //         point += vec2(0, tab_corner_radius);

        //     //         float d = distance(pixel_pos, point) - tab_corner_radius;
        //     //         alpha -= smoothstep(-0.5, 0.5, 1.0 - d);
        //     //     }
        //     // }
        //     // if (pixel_pos.x > bottom_right.x) {
        //     //     if (pixel_pos.y > bottom_right.y) {
        //     //         // discard;
        //     //     } else {
        //     //         vec2 point = bg_center + vec2(bg_size.x / 2, -bg_size.y / 2);
        //     //         // point += vec2(0, tab_corner_radius);

        //     //         float d = distance(pixel_pos, point) - tab_corner_radius;
        //     //         // alpha -= smoothstep(-0.5, 0.5, 1.0 - d);

        //     //         if (d > 0) {
        //     //             alpha = 0;
        //     //         }

        //     //         // if (d < -1 || d >= 0) {
        //     //         //     alpha = 0;
        //     //         // }
        //     //     }
        //     // }
        //     if (pixel_pos.x < bottom_left.x && pixel_pos.y > bottom_left.y) {
        //         float d = distance(pixel_pos, bottom_left) - tab_corner_radius;
        //         // alpha -= smoothstep(-0.5, 0.5, d);

        //         if (-2 < d && d < 0) {
        //             temp = vec3(1.0, 0.0, 0.0);
        //         } else if (d > 0) {
        //             // FIXME: This doesn't blend into the background color.
        //             alpha = 0;
        //         }
        //     }
        //     if (pixel_pos.x < top_left.x && pixel_pos.y > top_left.y) {
        //         float d = distance(pixel_pos, top_left) - tab_corner_radius;
        //         // alpha -= smoothstep(-0.5, 0.5, d);

        //         if (-2 < d && d < 0) {
        //             temp = vec3(1.0, 0.0, 0.0);
        //         } else if (d > 0) {
        //             // FIXME: This doesn't blend into the background color.
        //             alpha = 0;
        //         }
        //     }
        //     if (pixel_pos.x > top_right.x && pixel_pos.y > top_right.y) {
        //         float d = distance(pixel_pos, top_right) - tab_corner_radius;
        //         // alpha -= smoothstep(-0.5, 0.5, d);

        //         if (-2 < d && d < 0) {
        //             temp = vec3(1.0, 0.0, 0.0);
        //         } else if (d > 0) {
        //             // FIXME: This doesn't blend into the background color.
        //             alpha = 0;
        //         }
        //     }

        //     // float border_thickness = 2;
        //     // float border_left = bg_center.x - bg_size.x / 2 + border_thickness;
        //     // float border_right = bg_center.x + bg_size.x / 2 - border_thickness;
        //     // float border_top = bg_center.y + bg_size.y / 2 - border_thickness;
        //     // float border_bottom = bg_center.y - bg_size.y / 2 + border_thickness;
        //     // if (pixel_pos.x < border_left) {
        //     //     temp = vec3(1.0, 0.0, 0.0);
        //     // }
        //     // if (pixel_pos.x > border_right) {
        //     //     temp = vec3(1.0, 0.0, 0.0);
        //     // }
        //     // if (pixel_pos.y > border_top) {
        //     //     temp = vec3(1.0, 0.0, 0.0);
        //     // }
        //     // if (pixel_pos.y < border_bottom) {
        //     //     temp = vec3(1.0, 0.0, 0.0);
        //     // }
        // }

        alpha_mask = vec4(1.0);
        // color = vec4(bg_color.rgb * bg_color.a, bg_color.a);
        // color = vec4(0.89, 0.902, 0.91, bg_color.a);
        color = vec4(temp, alpha);
    }

    if (rendering_pass == 1) {
        vec4 texel = texture(mask, tex_coords);

        int colored = int(text_color.a);
        if (colored == 1) {
            // Revert alpha premultiplication.
            if (texel.a != 0.0) {
                texel.rgb /= texel.a;
            }

            alpha_mask = vec4(texel.a);
            color = vec4(texel.rgb, 1.0);
        } else {
            alpha_mask = vec4(texel.rgb, texel.r);
            color = vec4(text_color.rgb, 1.0);
        }
    }
}
