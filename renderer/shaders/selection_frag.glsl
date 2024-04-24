R"(

#version 330 core

flat in vec2 bg_center;
flat in vec2 bg_size;
flat in vec4 bg_color;
flat in vec4 bg_border_color;

layout(location = 0, index = 0) out vec4 color;
layout(location = 0, index = 1) out vec4 alpha_mask;

// Border flags.
#define BOTTOM_LEFT_INWARDS 1
#define BOTTOM_RIGHT_INWARDS 2
#define TOP_LEFT_INWARDS 4
#define TOP_RIGHT_INWARDS 8
#define BOTTOM_LEFT_OUTWARDS 16
#define BOTTOM_RIGHT_OUTWARDS 32
#define TOP_LEFT_OUTWARDS 64
#define TOP_RIGHT_OUTWARDS 128

void main() {
    if (bg_color.a == 0.0) discard;

    bool should_discard = true;
    
    vec3 computed_color = vec3(0);
    // vec3 computed_color = bg_color.rgb;
    float computed_alpha = 0.0;
    // float computed_alpha = 1.0;
    // TODO: Turn these into uniforms.
    float tab_corner_radius = 6;
    float border_thickness = 2;

    int border_flags = int(bg_border_color.a);
    bool has_bottom_left_inwards_border = (border_flags & BOTTOM_LEFT_INWARDS) == BOTTOM_LEFT_INWARDS;
    bool has_bottom_right_inwards_border = (border_flags & BOTTOM_RIGHT_INWARDS) == BOTTOM_RIGHT_INWARDS;
    bool has_top_left_inwards_border = (border_flags & TOP_LEFT_INWARDS) == TOP_LEFT_INWARDS;
    bool has_top_right_inwards_border = (border_flags & TOP_RIGHT_INWARDS) == TOP_RIGHT_INWARDS;
    bool has_bottom_left_outwards_border = (border_flags & BOTTOM_LEFT_OUTWARDS) == BOTTOM_LEFT_OUTWARDS;
    bool has_bottom_right_outwards_border = (border_flags & BOTTOM_RIGHT_OUTWARDS) == BOTTOM_RIGHT_OUTWARDS;
    bool has_top_left_outwards_border = (border_flags & TOP_LEFT_OUTWARDS) == TOP_LEFT_OUTWARDS;
    bool has_top_right_outwards_border = (border_flags & TOP_RIGHT_OUTWARDS) == TOP_RIGHT_OUTWARDS;

    vec2 pixel_pos = gl_FragCoord.xy;

    vec2 bottom_left = bg_center - bg_size / 2;
    vec2 bottom_right = bg_center + vec2(bg_size.x / 2, -bg_size.y / 2);
    vec2 top_left = bg_center + vec2(-bg_size.x / 2, bg_size.y / 2);
    vec2 top_right = bg_center + bg_size / 2;

    bottom_left.x += tab_corner_radius + border_thickness;
    bottom_right.x -= tab_corner_radius + border_thickness;
    top_left.x += tab_corner_radius + border_thickness;
    top_right.x -= tab_corner_radius + border_thickness;

    vec2 curve_bottom_left = bottom_left + tab_corner_radius;
    vec2 curve_bottom_right = bottom_right + vec2(-tab_corner_radius, tab_corner_radius);
    vec2 curve_top_left = top_left + vec2(tab_corner_radius, -tab_corner_radius);
    vec2 curve_top_right = top_right + vec2(-tab_corner_radius, -tab_corner_radius);

    curve_bottom_left.x -= border_thickness;
    curve_bottom_right.x += border_thickness;
    curve_top_left.x -= border_thickness;
    curve_top_right.x += border_thickness;

    float d = 0;
    if (has_bottom_left_inwards_border && pixel_pos.x < curve_bottom_left.x &&
        pixel_pos.y < curve_bottom_left.y) {
        d = distance(pixel_pos, curve_bottom_left) - tab_corner_radius;
    }
    if (has_bottom_right_inwards_border && pixel_pos.x > curve_bottom_right.x &&
        pixel_pos.y < curve_bottom_right.y) {
        d = distance(pixel_pos, curve_bottom_right) - tab_corner_radius;
    }
    if (has_top_left_inwards_border && pixel_pos.x < curve_top_left.x &&
        pixel_pos.y > curve_top_left.y) {
        d = distance(pixel_pos, curve_top_left) - tab_corner_radius;
    }
    if (has_top_right_inwards_border && pixel_pos.x > curve_top_right.x &&
        pixel_pos.y > curve_top_right.y) {
        d = distance(pixel_pos, curve_top_right) - tab_corner_radius;
    }
    if (-border_thickness < d && d < 0) {
        computed_color = bg_border_color.rgb;
        computed_alpha = 1.0;
    }

    // Temporary.
    vec2 curve_bottom_left_outwards = bottom_left + vec2(-tab_corner_radius, tab_corner_radius);
    vec2 curve_bottom_right_outwards = bottom_right + vec2(tab_corner_radius, tab_corner_radius);
    vec2 curve_top_left_outwards = top_left + vec2(-tab_corner_radius, -tab_corner_radius);
    vec2 curve_top_right_outwards = top_right + vec2(tab_corner_radius, -tab_corner_radius);

    d = 0;
    if (has_bottom_left_outwards_border && pixel_pos.x < curve_bottom_left_outwards.x + tab_corner_radius && pixel_pos.y < curve_bottom_left_outwards.y) {
        d = distance(pixel_pos, curve_bottom_left_outwards) - tab_corner_radius;
    }
    if (has_bottom_right_outwards_border && pixel_pos.x > curve_bottom_right_outwards.x - tab_corner_radius && pixel_pos.y < curve_bottom_right_outwards.y) {
        d = distance(pixel_pos, curve_bottom_right_outwards) - tab_corner_radius;
    }
    if (has_top_left_outwards_border && pixel_pos.x < curve_top_left_outwards.x + tab_corner_radius && pixel_pos.y > curve_top_left_outwards.y) {
        d = distance(pixel_pos, curve_top_left_outwards) - tab_corner_radius;
    }
    if (has_top_right_outwards_border && pixel_pos.x > curve_top_right_outwards.x - tab_corner_radius && pixel_pos.y > curve_top_right_outwards.y) {
        d = distance(pixel_pos, curve_top_right_outwards) - tab_corner_radius;
    }
    if (-border_thickness < d && d < 0) {
        computed_color = bg_border_color.rgb;
        computed_alpha = 1.0;
    }

    float left_edge = bg_center.x - bg_size.x / 2;
    float right_edge = bg_center.x + bg_size.x / 2;
    float bottom_edge = bg_center.y - bg_size.y / 2;
    float top_edge = bg_center.y + bg_size.y / 2;

    float border_left = left_edge + border_thickness;
    float border_right = right_edge - border_thickness;
    float border_top = top_edge - border_thickness;
    float border_bottom = bottom_edge + border_thickness;

    border_left += tab_corner_radius;
    border_right -= tab_corner_radius;

    if (border_left - border_thickness < pixel_pos.x && pixel_pos.x < border_left) {
        if (curve_bottom_left.y < pixel_pos.y && pixel_pos.y < curve_top_left.y) {
            computed_color = bg_border_color.rgb;
            computed_alpha = 1.0;
        }
    }
    if (border_right < pixel_pos.x && pixel_pos.x < border_right + border_thickness) {
        if (curve_bottom_right.y < pixel_pos.y && pixel_pos.y < curve_top_right.y) {
            computed_color = bg_border_color.rgb;
            computed_alpha = 1.0;
        }
    }
    if (pixel_pos.y < border_bottom) {
        if (curve_bottom_left.x < pixel_pos.x && pixel_pos.x < curve_bottom_right.x) {
            computed_color = bg_border_color.rgb;
            computed_alpha = 1.0;
        }
    }
    // if (pixel_pos.y > border_top) {
    //     if (curve_top_left.x < pixel_pos.x && pixel_pos.x < curve_top_right.x) {
    //         computed_color = bg_border_color.rgb;
    //         computed_alpha = 1.0;
    //     }
    // }

    alpha_mask = vec4(1.0);
    // FIXME: A `computed_alpha` of 0 still doesn't blend properly into the background color.
    color = vec4(computed_color, computed_alpha);
}

)"
