R"(

#version 330 core

flat in vec2 center;
flat in vec2 size;
flat in vec4 color;
flat in vec4 border_color;
flat in int border_flags;
flat in int bottom_border_offset;
flat in int top_border_offset;

uniform int rendering_pass;
uniform int r;
uniform int border_thickness;

layout(location = 0, index = 0) out vec4 out_color;
layout(location = 0, index = 1) out vec4 out_alpha_mask;

// Border flags.
#define LEFT 1
#define RIGHT 2
#define BOTTOM 4
#define TOP 8
#define BOTTOM_LEFT_INWARDS 16
#define BOTTOM_RIGHT_INWARDS 32
#define TOP_LEFT_INWARDS 64
#define TOP_RIGHT_INWARDS 128
#define BOTTOM_LEFT_OUTWARDS 256
#define BOTTOM_RIGHT_OUTWARDS 512
#define TOP_LEFT_OUTWARDS 1024
#define TOP_RIGHT_OUTWARDS 2048

void main() {
    vec3 computed_color;
    float computed_alpha;

    if (rendering_pass == 0) {
        computed_color = color.rgb;
        computed_alpha = 1.0;
    }

    if (rendering_pass == 1) {
        computed_color = border_color.rgb;
        computed_alpha = 0.0;

        bool has_left_border = (border_flags & LEFT) == LEFT;
        bool has_right_border = (border_flags & RIGHT) == RIGHT;
        bool has_bottom_border = (border_flags & BOTTOM) == BOTTOM;
        bool has_top_border = (border_flags & TOP) == TOP;
        bool has_bottom_left_inwards_border = (border_flags & BOTTOM_LEFT_INWARDS) == BOTTOM_LEFT_INWARDS;
        bool has_bottom_right_inwards_border = (border_flags & BOTTOM_RIGHT_INWARDS) == BOTTOM_RIGHT_INWARDS;
        bool has_top_left_inwards_border = (border_flags & TOP_LEFT_INWARDS) == TOP_LEFT_INWARDS;
        bool has_top_right_inwards_border = (border_flags & TOP_RIGHT_INWARDS) == TOP_RIGHT_INWARDS;
        bool has_bottom_left_outwards_border = (border_flags & BOTTOM_LEFT_OUTWARDS) == BOTTOM_LEFT_OUTWARDS;
        bool has_bottom_right_outwards_border = (border_flags & BOTTOM_RIGHT_OUTWARDS) == BOTTOM_RIGHT_OUTWARDS;
        bool has_top_left_outwards_border = (border_flags & TOP_LEFT_OUTWARDS) == TOP_LEFT_OUTWARDS;
        bool has_top_right_outwards_border = (border_flags & TOP_RIGHT_OUTWARDS) == TOP_RIGHT_OUTWARDS;

        bool has_bottom_left_border = has_bottom_left_inwards_border || has_bottom_left_outwards_border;
        bool has_bottom_right_border = has_bottom_right_inwards_border || has_bottom_right_outwards_border;
        bool has_top_left_border = has_top_left_inwards_border || has_top_left_outwards_border;
        bool has_top_right_border = has_top_right_inwards_border || has_top_right_outwards_border;

        vec2 pixel_pos = gl_FragCoord.xy;

        vec2 bottom_left = center - size / 2;
        vec2 bottom_right = center + vec2(size.x / 2, -size.y / 2);
        vec2 top_left = center + vec2(-size.x / 2, size.y / 2);
        vec2 top_right = center + size / 2;

        bottom_left.x += r + border_thickness;
        bottom_right.x -= r + border_thickness;
        top_left.x += r + border_thickness;
        top_right.x -= r + border_thickness;

        vec2 curve_bottom_left = bottom_left + r;
        vec2 curve_bottom_right = bottom_right + vec2(-r, r);
        vec2 curve_top_left = top_left + vec2(r, -r);
        vec2 curve_top_right = top_right + vec2(-r, -r);

        curve_bottom_left.x -= border_thickness;
        curve_bottom_right.x += border_thickness;
        curve_top_left.x -= border_thickness;
        curve_top_right.x += border_thickness;

        float d = 0;
        if (has_bottom_left_inwards_border && pixel_pos.x < curve_bottom_left.x &&
            pixel_pos.y < curve_bottom_left.y) {
            d = distance(pixel_pos, curve_bottom_left) - r;
        }
        if (has_bottom_right_inwards_border && pixel_pos.x > curve_bottom_right.x &&
            pixel_pos.y < curve_bottom_right.y) {
            d = distance(pixel_pos, curve_bottom_right) - r;
        }
        if (has_top_left_inwards_border && pixel_pos.x < curve_top_left.x &&
            pixel_pos.y > curve_top_left.y) {
            d = distance(pixel_pos, curve_top_left) - r;
        }
        if (has_top_right_inwards_border && pixel_pos.x > curve_top_right.x &&
            pixel_pos.y > curve_top_right.y) {
            d = distance(pixel_pos, curve_top_right) - r;
        }
        if (-border_thickness < d && d < 0) {
            computed_color = border_color.rgb;
            computed_alpha = 1.0;
        }

        vec2 curve_bottom_left_outwards = bottom_left + vec2(-r, r);
        vec2 curve_bottom_right_outwards = bottom_right + vec2(r, r);
        vec2 curve_top_left_outwards = top_left + vec2(-r, -r);
        vec2 curve_top_right_outwards = top_right + vec2(r, -r);

        d = 0;
        if (has_bottom_left_outwards_border && pixel_pos.x < curve_bottom_left_outwards.x + r && pixel_pos.y < curve_bottom_left_outwards.y) {
            d = distance(pixel_pos, curve_bottom_left_outwards) - r;
        }
        if (has_bottom_right_outwards_border && pixel_pos.x > curve_bottom_right_outwards.x - r && pixel_pos.y < curve_bottom_right_outwards.y) {
            d = distance(pixel_pos, curve_bottom_right_outwards) - r;
        }
        if (has_top_left_outwards_border && pixel_pos.x < curve_top_left_outwards.x + r && pixel_pos.y > curve_top_left_outwards.y) {
            d = distance(pixel_pos, curve_top_left_outwards) - r;
        }
        if (has_top_right_outwards_border && pixel_pos.x > curve_top_right_outwards.x - r && pixel_pos.y > curve_top_right_outwards.y) {
            d = distance(pixel_pos, curve_top_right_outwards) - r;
        }
        if (-border_thickness < d && d < 0) {
            computed_color = border_color.rgb;
            computed_alpha = 1.0;
        }

        float left_edge = center.x - size.x / 2;
        float right_edge = center.x + size.x / 2;
        float bottom_edge = center.y - size.y / 2;
        float top_edge = center.y + size.y / 2;

        float border_left = left_edge + border_thickness;
        float border_right = right_edge - border_thickness;
        float border_top = top_edge - border_thickness;
        float border_bottom = bottom_edge + border_thickness;

        border_left += r;
        border_right -= r;

        if (has_left_border && border_left - border_thickness < pixel_pos.x && pixel_pos.x < border_left) {
            if (!(has_bottom_left_border && pixel_pos.y < curve_bottom_left.y) &&
                !(has_top_left_border && pixel_pos.y > curve_top_left.y)) {
                computed_color = border_color.rgb;
                computed_alpha = 1.0;
            }
        }
        if (has_right_border && border_right < pixel_pos.x && pixel_pos.x < border_right + border_thickness) {
            if (!(has_bottom_right_border && pixel_pos.y < curve_bottom_right.y) &&
                !(has_top_right_border && pixel_pos.y > curve_top_right.y)) {
                computed_color = border_color.rgb;
                computed_alpha = 1.0;
            }
        }

        bool is_past_bottom_offset = pixel_pos.x > border_left + bottom_border_offset;
        bool is_past_top_offset = pixel_pos.x > border_left + top_border_offset;

        if (has_bottom_border && pixel_pos.y < border_bottom && is_past_bottom_offset) {
            if (!(has_bottom_left_border && pixel_pos.x < curve_bottom_left.x) &&
                !(has_bottom_right_border && pixel_pos.x > curve_bottom_right.x)) {
                computed_color = border_color.rgb;
                computed_alpha = 1.0;
            }
        }
        if (has_top_border && pixel_pos.y > border_top && is_past_top_offset) {
            if (!(has_top_left_border && pixel_pos.x < curve_top_left.x) &&
                !(has_top_right_border && pixel_pos.x > curve_top_right.x)) {
                computed_color = border_color.rgb;
                computed_alpha = 1.0;
            }
        }

        if (computed_alpha == 0.0) {
            discard;
        }
    }

    out_alpha_mask = vec4(1.0);
    out_color = vec4(computed_color, computed_alpha);
}

)"
