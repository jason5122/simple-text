#include <metal_stdlib>
#include <simd/simd.h>
using namespace metal;

// Include header shared between this Metal shader code and C code executing Metal API commands
#include "ShaderTypes.h"

// Vertex shader outputs and per-fragment inputs
struct RasterizerData {
    float4 clipSpacePosition [[position]];
    float3 color;
};

float2 normalize(float2 point, float2 viewportSize) {
    float2 inverseViewportSize = 1 / (viewportSize / 2.0);
    float clipX = (point.x * inverseViewportSize.x) - 1.0f;
    float clipY = (point.y * inverseViewportSize.y) - 1.0f;
    return float2(clipX, clipY);
}

vertex RasterizerData vertexShader(uint vertexID [[vertex_id]],
                                   constant Vertex* vertexArray
                                   [[buffer(VertexInputIndexVertices)]],
                                   constant Uniforms& uniforms
                                   [[buffer(VertexInputIndexUniforms)]]) {
    RasterizerData out;

    float2 pixelSpacePosition = vertexArray[vertexID].position.xy;
    float2 viewportSize = float2(uniforms.viewportSize);

    out.clipSpacePosition.xy = normalize(pixelSpacePosition, viewportSize);
    out.clipSpacePosition.z = 0.0;
    out.clipSpacePosition.w = 1.0;

    out.color = vertexArray[vertexID].color;

    return out;
}

fragment float4 fragmentShader(RasterizerData in [[stage_in]]) {
    return float4(in.color, 1.0);
}
