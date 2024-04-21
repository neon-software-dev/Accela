/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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

// Vertex Data
layout(location = 0) in vec3 i_vertexPosition_modelSpace;
layout(location = 1) in vec3 i_vertexNormal_modelSpace;
layout(location = 2) in vec2 i_vertexUv;
layout(location = 3) in vec3 i_vertexTangent_modelSpace;

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

//
// OUTPUTS
//
layout(location = 0) out int o_instanceIndex;
layout(location = 1) out vec3 o_vertexNormal_modelSpace;
layout(location = 2) out vec2 o_vertexUv;
layout(location = 3) out vec3 o_vertexTangent_modelSpace;

void main()
{
    gl_Position = vec4(i_vertexPosition_modelSpace, 1.0);

    o_instanceIndex = gl_InstanceIndex;
    o_vertexNormal_modelSpace = i_vertexNormal_modelSpace;
    o_vertexUv = i_vertexUv;
    o_vertexTangent_modelSpace = i_vertexTangent_modelSpace;
}
