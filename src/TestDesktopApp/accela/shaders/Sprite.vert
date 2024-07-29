/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#version 460

//
// Definitions
//
struct GlobalPayload
{
    // General
    mat4 surfaceTransform;          // Projection Space -> Rotated projection space

    // Lighting
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
};

struct SpritePayload
{
    mat4 modelTransform;
    vec2 uvTranslation;
    vec2 uvSize;
};

//
// Internal
//
vec2 GetUVCoords(const SpritePayload spritePayload, const uint vertexIndex);

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

layout(set = 0, binding = 1) uniform ViewProjectionPayloadUniform
{
    ViewProjectionPayload data;
} u_viewProjectionData;

// Set 1 - Sprite Data
layout(set = 1, binding = 0) readonly buffer SpritePayloadBuffer
{
    SpritePayload data[];
} i_spriteData;

// Set 3 - Draw Data
layout(set = 3, binding = 0) readonly buffer DrawPayloadBuffer
{
    DrawPayload data[];
} i_drawData;

//
// OUTPUTS
//
layout(location = 0) out vec2 o_fragTexCoord;           // The vertex's tex coord

void main()
{
    const DrawPayload drawPayload = i_drawData.data[gl_InstanceIndex];
    const SpritePayload spritePayload = i_spriteData.data[drawPayload.dataIndex];

    const uint vertexIndex = gl_VertexIndex % 4;

    // Final MVP position of this vertex
    gl_Position =
        u_globalData.data.surfaceTransform *
        u_viewProjectionData.data.projectionTransform *
        u_viewProjectionData.data.viewTransform *
        spritePayload.modelTransform *
        vec4(i_vertexPosition_modelSpace, 1.0f);

    // Outputs being passed to the fragment shader
    o_fragTexCoord = GetUVCoords(spritePayload, vertexIndex);
}

vec2 GetUVCoords(const SpritePayload spritePayload, const uint vertexIndex)
{
    vec2 uvCoord = spritePayload.uvTranslation;

    if (vertexIndex == 1)
    {
        uvCoord.x += spritePayload.uvSize.x;
    }
    else if (vertexIndex == 2)
    {
        uvCoord.x += spritePayload.uvSize.x;
        uvCoord.y += spritePayload.uvSize.y;
    }
    else if (vertexIndex == 3)
    {
        uvCoord.y += spritePayload.uvSize.y;
    }

    return uvCoord;
}
