/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#version 460

//
// Definitions
//
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
// Inputs
//

layout(location = 0) flat in int i_instanceIndex[];
layout(location = 1) in vec3 i_vertexNormal_modelSpace[];
layout(location = 2) in vec2 i_vertexUv[];
layout(location = 3) in vec3 i_vertexTangent_modelSpace[];

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
// Outputs
//
// Tesselation Control output specification
layout(vertices = 4) out;

layout(location = 0) out int o_instanceIndex[4];
layout(location = 1) out vec3 o_vertexNormal_modelSpace[4];
layout(location = 2) out vec2 o_vertexUv[4];
layout(location = 3) out vec3 o_vertexTangent_modelSpace[4];

void main()
{
    const DrawPayload drawPayload = i_drawData.data[i_instanceIndex[0]];
    const TerrainPayload terrainPayload = i_terrainData.data[drawPayload.dataIndex];

    if (gl_InvocationID == 0)
    {
        gl_TessLevelInner[0] = terrainPayload.tesselationLevel;
        gl_TessLevelInner[1] = terrainPayload.tesselationLevel;
        gl_TessLevelOuter[0] = terrainPayload.tesselationLevel;
        gl_TessLevelOuter[1] = terrainPayload.tesselationLevel;
        gl_TessLevelOuter[2] = terrainPayload.tesselationLevel;
        gl_TessLevelOuter[3] = terrainPayload.tesselationLevel;
    }

    gl_out[gl_InvocationID].gl_Position =  gl_in[gl_InvocationID].gl_Position;

    o_instanceIndex[gl_InvocationID] = i_instanceIndex[gl_InvocationID];
    o_vertexNormal_modelSpace[gl_InvocationID] = i_vertexNormal_modelSpace[gl_InvocationID];
    o_vertexUv[gl_InvocationID] = i_vertexUv[gl_InvocationID];
    o_vertexTangent_modelSpace[gl_InvocationID] = i_vertexTangent_modelSpace[gl_InvocationID];
}