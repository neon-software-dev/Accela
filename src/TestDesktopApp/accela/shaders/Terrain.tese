/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#version 460
#extension GL_EXT_multiview : require

//
// Definitions
//
struct GlobalPayload
{
    // General
    mat4 surfaceTransform;          // Projection Space -> Rotated projection space

    // Lighting
    uint numLights;
    float ambientLightIntensity;
    vec3 ambientLightColor;
    float shadowCascadeOverlap;
};

struct ViewProjectionPayload
{
    mat4 viewTransform;             // Global World Space -> View Space transform matrix
    mat4 projectionTransform;       // Global View Space -> Projection Space transform matrix
};

struct DrawPayload
{
    uint dataIndex;
    uint materialIndex;
};

struct TerrainPayload
{
    mat4 modelTransform;
    float tesselationLevel;
    float displacementFactor;
};

//
// INPUTS
//

// Tesselation Evaluation inputs (via gl_TessCoord)
layout(quads, equal_spacing, ccw) in;

// Patch control point inputs
layout(location = 0) flat in int i_instanceIndex[];
layout(location = 1) in vec3 i_vertexNormal_modelSpace[];
layout(location = 2) in vec2 i_vertexUv[];
layout(location = 3) in vec3 i_vertexTangent_modelSpace[];

// Set 0 - Global Data
layout(set = 0, binding = 0) uniform GlobalPayloadUniform
{
    GlobalPayload data;
} u_globalData;

layout(set = 0, binding = 1) readonly buffer ViewProjectionPayloadUniform
{
    ViewProjectionPayload data[];
} i_viewProjectionData;

// Set 1 - Terrain Data
layout(set = 1, binding = 0) readonly buffer TerrainPayloadBuffer
{
    TerrainPayload data[];
} i_terrainData;

// Set 3 - Draw Data
layout(set = 3, binding = 0) readonly buffer DrawPayloadBuffer
{
    DrawPayload data[];
} i_drawData;

layout(set = 3, binding = 1) uniform sampler2D i_heightSampler;

//
// Outputs
//
layout(location = 0) out int o_instanceIndex;
layout(location = 1) out vec2 o_vertexUv;
layout(location = 2) out vec3 o_vertexNormal_modelSpace;
layout(location = 3) out vec3 o_vertexPosition_worldSpace;
layout(location = 4) out mat3 o_tbnNormalTransform;

mat3 GenerateTBNNormalTransform(vec3 vertexNormal_modelSpace, vec3 vertexTangent_modelSpace)
{
    vec3 T =  normalize(vertexTangent_modelSpace);
    vec3 N = normalize(vertexNormal_modelSpace);

    T = normalize(T - dot(T, N) * N); // Re-orthogonalize T with respect to N
    vec3 B = normalize(cross(N, T));  // Then retrieve bitangent vector B with the cross product of T and N

    return mat3(T, B, N);
}

// MixToPatchCenter funcs determine patch middle value that's a mix of the provided point
// values. Requires vectors p0->p3 and p1->p2 to be horizontal lines within the patch.
vec4 MixToTessCoord(vec4 p0, vec4 p1, vec4 p2, vec4 p3)
{
    vec4 xP1 = mix(p0, p3, gl_TessCoord.x);
    vec4 xP2 = mix(p1, p2, gl_TessCoord.x);
    return mix(xP1, xP2, gl_TessCoord.y);
}

vec2 MixToTessCoord(vec2 p0, vec2 p1, vec2 p2, vec2 p3)
{
    vec2 xP1 = mix(p0, p3, gl_TessCoord.x);
    vec2 xP2 = mix(p1, p2, gl_TessCoord.x);
    return mix(xP1, xP2, gl_TessCoord.y);
}

vec3 MixToTessCoord(vec3 p0, vec3 p1, vec3 p2, vec3 p3)
{
    vec3 xP1 = mix(p0, p3, gl_TessCoord.x);
    vec3 xP2 = mix(p1, p2, gl_TessCoord.x);
    return mix(xP1, xP2, gl_TessCoord.y);
}

// Samples the height map and adjusts the provided position upwards by the height and by the deplacement factor
vec4 GetPointHeightMapped(vec4 position_modelSpace, vec3 normal_modelSpace, float displacementFactor, vec2 uv)
{
    return
        position_modelSpace +
        (vec4(normal_modelSpace, 0) * displacementFactor * texture(i_heightSampler, uv).r);
}

void main()
{
    const DrawPayload drawPayload = i_drawData.data[i_instanceIndex[0]];
    const TerrainPayload terrainPayload = i_terrainData.data[drawPayload.dataIndex];

    //
    // Mix the patch's four points together to determine center point values
    //
    const vec4 vertPosition_modelSpace = MixToTessCoord(
        gl_in[0].gl_Position, gl_in[1].gl_Position, gl_in[2].gl_Position, gl_in[3].gl_Position);

    const vec3 vertNormal_modelSpace = MixToTessCoord(
        i_vertexNormal_modelSpace[0], i_vertexNormal_modelSpace[1], i_vertexNormal_modelSpace[2], i_vertexNormal_modelSpace[3]);

    const vec2 vertUv = MixToTessCoord(
        i_vertexUv[0], i_vertexUv[1], i_vertexUv[2], i_vertexUv[3]);

    const vec3 vertTangent_modelSpace = MixToTessCoord(
        i_vertexTangent_modelSpace[0], i_vertexTangent_modelSpace[1], i_vertexTangent_modelSpace[2], i_vertexTangent_modelSpace[3]);

    // Adjust the vertex's height by the sampled height map value
    const vec4 vertPositionAdjusted_modelSpace = GetPointHeightMapped(
        vertPosition_modelSpace,
        vertNormal_modelSpace,
        terrainPayload.displacementFactor,
        vertUv
    );

    const vec4 vertexPosition_worldSpace = terrainPayload.modelTransform * vertPositionAdjusted_modelSpace;

    gl_Position =
        u_globalData.data.surfaceTransform *
        i_viewProjectionData.data[gl_ViewIndex].projectionTransform *
        i_viewProjectionData.data[gl_ViewIndex].viewTransform *
        vertexPosition_worldSpace;

    o_instanceIndex = i_instanceIndex[0];
    o_vertexNormal_modelSpace = vertNormal_modelSpace;
    o_vertexUv = vertUv;
    o_vertexPosition_worldSpace = vec3(vertexPosition_worldSpace);
    o_tbnNormalTransform = GenerateTBNNormalTransform(vertNormal_modelSpace, vertTangent_modelSpace);
}