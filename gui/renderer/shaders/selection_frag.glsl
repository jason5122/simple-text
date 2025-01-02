R"(

#version 330 core

flat in vec2 center;
flat in vec2 size;
flat in vec4 color;
flat in vec4 border_color;
flat in int border_flags;
flat in int bottom_border_offset;
flat in int top_border_offset;
flat in int hide_background;
flat in vec4 clip_rect;

uniform int rendering_pass;
uniform int r;
uniform int thickness;
uniform vec2 resolution;

out vec4 out_color;

// Border flags.
const int kLeft = 1;
const int kRight = 1 << 1;
const int kBottom = 1 << 2;
const int kTop = 1 << 3;
const int kBottomLeftInwards = 1 << 4;
const int kBottomRightInwards = 1 << 5;
const int kTopLeftInwards = 1 << 6;
const int kTopRightInwards = 1 << 7;
const int kBottomLeftOutwards = 1 << 8;
const int kBottomRightOutwards = 1 << 9;
const int kTopLeftOutwards = 1 << 10;
const int kTopRightOutwards = 1 << 11;

void main() {
    vec2 coord = gl_FragCoord.xy;

    // This isn't perfect, but it's good enough for now.
    float min_x = clip_rect.x;
    float max_x = clip_rect.z;
    // `clip_rect` is <min_x, min_y, max_x, max_y>. However, since OpenGL is inverted, we swap
    // `min_y` and `max_y`, then subtract them from the resolution.
    float min_y = resolution.y - clip_rect.w;
    float max_y = resolution.y - clip_rect.y;
    if (!(min_x <= coord.x && coord.x <= max_x)) discard;
    if (!(min_y <= coord.y && coord.y <= max_y)) discard;
    
    bool has_l = (border_flags & kLeft) == kLeft;
    bool has_r = (border_flags & kRight) == kRight;
    bool has_b = (border_flags & kBottom) == kBottom;
    bool has_t = (border_flags & kTop) == kTop;
    bool has_bl_in = (border_flags & kBottomLeftInwards) == kBottomLeftInwards;
    bool has_br_in = (border_flags & kBottomRightInwards) == kBottomRightInwards;
    bool has_tl_in = (border_flags & kTopLeftInwards) == kTopLeftInwards;
    bool has_tr_in = (border_flags & kTopRightInwards) == kTopRightInwards;
    bool has_bl_out = (border_flags & kBottomLeftOutwards) == kBottomLeftOutwards;
    bool has_br_out = (border_flags & kBottomRightOutwards) == kBottomRightOutwards;
    bool has_tl_out = (border_flags & kTopLeftOutwards) == kTopLeftOutwards;
    bool has_tr_out = (border_flags & kTopRightOutwards) == kTopRightOutwards;

    bool has_bl = has_bl_in || has_bl_out;
    bool has_br = has_br_in || has_br_out;
    bool has_tl = has_tl_in || has_tl_out;
    bool has_tr = has_tr_in || has_tr_out;

    vec2 bl = center - size / 2;
    vec2 br = center + vec2(size.x / 2, -size.y / 2);
    vec2 tl = center + vec2(-size.x / 2, size.y / 2);
    vec2 tr = center + size / 2;

    bl.x += r + thickness;
    br.x -= r + thickness;
    tl.x += r + thickness;
    tr.x -= r + thickness;

    vec2 bl_in = bl + r;
    vec2 br_in = br + vec2(-r, r);
    vec2 tl_in = tl + vec2(r, -r);
    vec2 tr_in = tr + vec2(-r, -r);

    bl_in.x -= thickness;
    br_in.x += thickness;
    tl_in.x -= thickness;
    tr_in.x += thickness;

    vec2 bl_out = bl + vec2(-r, r);
    vec2 br_out = br + vec2(r, r);
    vec2 tl_out = tl + vec2(-r, -r);
    vec2 tr_out = tr + vec2(r, -r);

    float l_edge = center.x - size.x / 2;
    float r_edge = center.x + size.x / 2;
    float b_edge = center.y - size.y / 2;
    float t_edge = center.y + size.y / 2;

    float l_border = l_edge + thickness;
    float r_border = r_edge - thickness;
    float t_border = t_edge - thickness;
    float b_border = b_edge + thickness;

    l_border += r;
    r_border -= r;

    float d_in = 0;
    if (has_bl_in && coord.x < bl_in.x && coord.y < bl_in.y) {
        d_in = distance(coord, bl_in) - r;
    }
    if (has_br_in && coord.x > br_in.x && coord.y < br_in.y) {
        d_in = distance(coord, br_in) - r;
    }
    if (has_tl_in && coord.x < tl_in.x && coord.y > tl_in.y) {
        d_in = distance(coord, tl_in) - r;
    }
    if (has_tr_in && coord.x > tr_in.x && coord.y > tr_in.y) {
        d_in = distance(coord, tr_in) - r;
    }

    float d_out = 0;
    if (has_bl_out && coord.x < bl_out.x + r && coord.y < bl_out.y) {
        d_out = distance(coord, bl_out) - r;
    }
    if (has_br_out && coord.x > br_out.x - r && coord.y < br_out.y) {
        d_out = distance(coord, br_out) - r;
    }
    if (has_tl_out && coord.x < tl_out.x + r && coord.y > tl_out.y) {
        d_out = distance(coord, tl_out) - r;
    }
    if (has_tr_out && coord.x > tr_out.x - r && coord.y > tr_out.y) {
        d_out = distance(coord, tr_out) - r;
    }
    
    vec3 computed_color;
    float computed_alpha;

    if (rendering_pass == 0) {
        if (hide_background == 1) {
            discard;
        }
        
        computed_color = color.rgb;
        computed_alpha = 1.0;

        if (d_in > 0) {
            computed_alpha = 0.0;
        }
        if (d_out < -thickness) {
            computed_alpha = 0.0;
        }

        if (has_l && coord.x < l_border - thickness) {
            if (!(has_bl && coord.y < bl_in.y) && !(has_tl && coord.y > tl_in.y)) {
                computed_alpha = 0.0;
            }
        }
        if (has_r && coord.x > r_border + thickness) {
            if (!(has_br && coord.y < br_in.y) && !(has_tr && coord.y > tr_in.y)) {
                computed_alpha = 0.0;
            }
        }
    }

    if (rendering_pass == 1) {
        computed_color = border_color.rgb;
        computed_alpha = 0.0;

        if (-thickness < d_in && d_in < 0) {
            computed_alpha = 1.0;
        }
        if (-thickness < d_out && d_out < 0) {
            computed_alpha = 1.0;
        }

        if (has_l && l_border - thickness < coord.x && coord.x < l_border) {
            if (!(has_bl && coord.y < bl_in.y) && !(has_tl && coord.y > tl_in.y)) {
                computed_alpha = 1.0;
            }
        }
        if (has_r && r_border < coord.x && coord.x < r_border + thickness) {
            if (!(has_br && coord.y < br_in.y) && !(has_tr && coord.y > tr_in.y)) {
                computed_alpha = 1.0;
            }
        }

        bool is_past_bottom_offset = coord.x > l_border + bottom_border_offset;
        bool is_past_top_offset = coord.x > l_border + top_border_offset;

        if (has_b && coord.y < b_border && is_past_bottom_offset) {
            if (!(has_bl && coord.x < bl_in.x) && !(has_br && coord.x > br_in.x)) {
                computed_alpha = 1.0;
            }
        }
        if (has_t && coord.y > t_border && is_past_top_offset) {
            if (!(has_tl && coord.x < tl_in.x) && !(has_tr && coord.x > tr_in.x)) {
                computed_alpha = 1.0;
            }
        }
    }

    if (computed_alpha == 0.0) {
        discard;
    }

    out_color = vec4(computed_color, computed_alpha);
}

)"
