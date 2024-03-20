#version 330 core

in vec2 tex_coords;
flat in vec4 text_color;
flat in vec2 bg_center;
flat in vec2 bg_size;
flat in vec4 bg_color;
flat in vec4 bg_border_color;

layout(location = 0, index = 0) out vec4 color;
layout(location = 0, index = 1) out vec4 alpha_mask;

uniform sampler2D mask;
uniform int rendering_pass;

// Border flags.
#define LEFT 1
#define RIGHT 2
#define BOTTOM 4
#define TOP 8
#define BOTTOM_LEFT 16
#define BOTTOM_RIGHT 32
#define TOP_LEFT 64
#define TOP_RIGHT 128

void main() {
    if (rendering_pass == 0) {
        if (bg_color.a == 0.0) discard;

        vec3 computed_color = bg_color.rgb;
        float computed_alpha = 1.0;
        float tab_corner_radius = 6;

        int border_flags = int(bg_border_color.a);
        bool has_left_border = (border_flags & LEFT) == LEFT;
        bool has_right_border = (border_flags & RIGHT) == RIGHT;
        bool has_bottom_border = (border_flags & BOTTOM) == BOTTOM;
        bool has_top_border = (border_flags & TOP) == TOP;
        bool has_bottom_left_border = (border_flags & BOTTOM_LEFT) == BOTTOM_LEFT;
        bool has_bottom_right_border = (border_flags & BOTTOM_RIGHT) == BOTTOM_RIGHT;
        bool has_top_left_border = (border_flags & TOP_LEFT) == TOP_LEFT;
        bool has_top_right_border = (border_flags & TOP_RIGHT) == TOP_RIGHT;

        vec2 pixel_pos = gl_FragCoord.xy;

        vec2 bottom_left = bg_center - bg_size / 2;
        vec2 bottom_right = bg_center + vec2(bg_size.x / 2, -bg_size.y / 2);
        vec2 top_left = bg_center + vec2(-bg_size.x / 2, bg_size.y / 2);
        vec2 top_right = bg_center + bg_size / 2;

        vec2 curve_bottom_left = bottom_left + tab_corner_radius;
        vec2 curve_bottom_right = bottom_right + vec2(-tab_corner_radius, tab_corner_radius);
        vec2 curve_top_left = top_left + vec2(tab_corner_radius, -tab_corner_radius);
        vec2 curve_top_right = top_right + vec2(-tab_corner_radius, -tab_corner_radius);
        if (has_bottom_left_border && pixel_pos.x < curve_bottom_left.x &&
            pixel_pos.y < curve_bottom_left.y) {
            float d = distance(pixel_pos, curve_bottom_left) - tab_corner_radius;

            if (-2 < d && d < 0) {
                computed_color = bg_border_color.rgb;
            } else if (d > 0) {
                computed_alpha = 0;
            }
        }
        if (has_bottom_right_border && pixel_pos.x > curve_bottom_right.x &&
            pixel_pos.y < curve_bottom_right.y) {
            float d = distance(pixel_pos, curve_bottom_right) - tab_corner_radius;

            if (-2 < d && d < 0) {
                computed_color = bg_border_color.rgb;
            } else if (d > 0) {
                computed_alpha = 0;
            }
        }
        if (has_top_left_border && pixel_pos.x < curve_top_left.x &&
            pixel_pos.y > curve_top_left.y) {
            float d = distance(pixel_pos, curve_top_left) - tab_corner_radius;

            if (-2 < d && d < 0) {
                computed_color = bg_border_color.rgb;
            } else if (d > 0) {
                computed_alpha = 0;
            }
        }
        if (has_top_right_border && pixel_pos.x > curve_top_right.x &&
            pixel_pos.y > curve_top_right.y) {
            float d = distance(pixel_pos, curve_top_right) - tab_corner_radius;

            if (-2 < d && d < 0) {
                computed_color = bg_border_color.rgb;
            } else if (d > 0) {
                computed_alpha = 0;
            }
        }

        float border_thickness = 2;
        float border_left = bottom_left.x + border_thickness;
        float border_right = bottom_right.x - border_thickness;
        float border_top = top_left.y - border_thickness;
        float border_bottom = bottom_left.y + border_thickness;
        if (has_left_border && pixel_pos.x < border_left &&
            (curve_bottom_left.y < pixel_pos.y && pixel_pos.y < curve_top_left.y)) {
            computed_color = bg_border_color.rgb;
        }
        if (has_right_border && pixel_pos.x > border_right &&
            (curve_bottom_right.y < pixel_pos.y && pixel_pos.y < curve_top_right.y)) {
            computed_color = bg_border_color.rgb;
        }
        if (has_top_border && pixel_pos.y > border_top &&
            (curve_top_left.x < pixel_pos.x && pixel_pos.x < curve_top_right.x)) {
            computed_color = bg_border_color.rgb;
        }
        if (has_bottom_border && pixel_pos.y < border_bottom &&
            (curve_bottom_left.x < pixel_pos.x && pixel_pos.x < curve_bottom_right.x)) {
            computed_color = bg_border_color.rgb;
        }

        alpha_mask = vec4(1.0);
        // FIXME: A `computed_alpha` of 0 still doesn't blend properly into the background color.
        color = vec4(computed_color, computed_alpha);
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
