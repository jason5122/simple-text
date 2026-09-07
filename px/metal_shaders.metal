R"px(
// Metal Shading Language sources for metal_render_context. Compiled at runtime from this string,
// the same way gl_render_context builds its programs from the .glsl files, so no metallib step is
// needed in the build.
//
// Geometry is identical to the GL shaders: every primitive is a 4-vertex triangle strip whose
// corner is derived from vertex_id, expanded per instance from a plain instance array. The only
// coordinate difference is that Metal's texture origin and scissor rectangles are already top-left,
// so the y flip happens once, in the vertex shader, and nowhere else.

#include <metal_stdlib>
using namespace metal;

static inline float2 strip_corner(uint vertex_id) {
    return float2((vertex_id == 1 || vertex_id == 3) ? 1.0 : 0.0, vertex_id >= 2 ? 1.0 : 0.0);
}

static inline float4 clip_position(float2 device_position, float2 viewport) {
    const float2 ndc = device_position / viewport * 2.0 - 1.0;
    return float4(ndc.x, -ndc.y, 0.0, 1.0);
}

// ── solid rectangles ─────────────────────────────────────────────────────────────────────────────

struct rect_instance {
    float4 geometry;  // x, y, width, height in device pixels
    float4 color;     // premultiplied
};

struct rect_varyings {
    float4 position [[position]];
    float4 color [[flat]];
};

vertex rect_varyings px_rect_vertex(uint vertex_id [[vertex_id]],
                                    uint instance_id [[instance_id]],
                                    device const rect_instance* instances [[buffer(0)]],
                                    constant float2& viewport [[buffer(1)]]) {
    const rect_instance instance = instances[instance_id];
    const float2 position = instance.geometry.xy + strip_corner(vertex_id) * instance.geometry.zw;
    rect_varyings out;
    out.position = clip_position(position, viewport);
    out.color = instance.color;
    return out;
}

fragment float4 px_rect_fragment(rect_varyings in [[stage_in]]) {
    return in.color;
}

// ── glyphs ───────────────────────────────────────────────────────────────────────────────────────

struct glyph_instance {
    float4 color;        // straight (not premultiplied) tint
    float4 destination;  // x, y, width, height in device pixels
    float4 source;       // atlas x, atlas y; the last two lanes are reserved for fade/clip bounds
};

struct glyph_vertex_uniforms {
    float2 viewport;
    float texture_size;
    float padding;
};

struct glyph_fragment_uniforms {
    uint colored;
    uint alternate;
};

struct glyph_varyings {
    float4 position [[position]];
    float2 uv;
    float4 color [[flat]];
};

vertex glyph_varyings px_glyph_vertex(uint vertex_id [[vertex_id]],
                                      uint instance_id [[instance_id]],
                                      device const glyph_instance* instances [[buffer(0)]],
                                      constant glyph_vertex_uniforms& uniforms [[buffer(1)]]) {
    const glyph_instance instance = instances[instance_id];
    const float2 corner = strip_corner(vertex_id);
    const float2 position = instance.destination.xy + corner * instance.destination.zw;
    glyph_varyings out;
    out.position = clip_position(position, uniforms.viewport);
    out.uv = (instance.source.xy + corner * instance.destination.zw) / uniforms.texture_size;
    out.color = instance.color;
    return out;
}

// Dual-source output. For monochrome glyphs the pipeline blends with
// (source1, 1 - source1) so each channel gets its own coverage, which is how the software
// compositor and the GL path treat LCD-filtered tiles. Colored glyphs use ordinary premultiplied
// source-over and ignore the second output.
struct glyph_output {
    float4 color [[color(0), index(0)]];
    float4 coverage [[color(0), index(1)]];
};

fragment glyph_output px_glyph_fragment(glyph_varyings in [[stage_in]],
                                        texture2d<float> atlas [[texture(0)]],
                                        constant glyph_fragment_uniforms& uniforms [[buffer(0)]]) {
    constexpr sampler nearest(coord::normalized, filter::nearest, address::clamp_to_edge);
    float4 sample_color = atlas.sample(nearest, in.uv);
    if (uniforms.alternate != 0 && uniforms.colored == 0) {
        // The alternate cache stores monochrome glyphs at reversed polarity: black ink on an
        // opaque white background. Recover coverage the same way composite_glyph_scanline does,
        // leaving the rasterized alpha alone.
        sample_color.rgb = 1.0 - sample_color.rgb;
    }
    glyph_output out;
    if (uniforms.colored != 0) {
        out.color = sample_color * in.color.a;
        out.coverage = float4(sample_color.a * in.color.a);
    } else {
        out.color = in.color;
        out.coverage = sample_color * in.color.a;
    }
    return out;
}
)px"
