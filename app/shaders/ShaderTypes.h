#pragma once

#include <simd/simd.h>

typedef enum VertexInputIndex {
    VertexInputIndexVertices = 0,
    VertexInputIndexUniforms = 1,
} VertexInputIndex;

typedef struct {
    // Positions in pixel space (i.e. a value of 100 indicates 100 pixels from the origin/center)
    vector_float2 position;

    // 2D texture coordinate
    vector_float3 color;
} Vertex;

typedef struct {
    vector_uint2 viewportSize;
} Uniforms;
