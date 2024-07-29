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

struct ObjectPayload
{
    mat4 modelTransform;
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

// Set 1 - Object Data
layout(set = 1, binding = 0) readonly buffer ObjectPayloadBuffer
{
    ObjectPayload data[];
} i_objectData;

// Set 3 - Draw Data
layout(set = 3, binding = 0) readonly buffer DrawPayloadBuffer
{
    DrawPayload data[];
} i_drawData;

// Push Constants
layout(push_constant) uniform constants
{
    uint shadowMapType;
    float lightMaxAffectRange;
} PushConstants;

//
// OUTPUTS
//
layout(location = 0) out vec3 o_vertexPosition_shadowViewSpace;
layout(location = 1) out int o_instanceIndex;
layout(location = 2) out vec2 o_fragTexCoord;

void main()
{
    const DrawPayload drawPayload = i_drawData.data[gl_InstanceIndex];
    const ObjectPayload objectPayload = i_objectData.data[drawPayload.dataIndex];

    const vec4 vertexPosition_worldSpace =
        objectPayload.modelTransform *
        vec4(i_vertexPosition_modelSpace, 1.0f);

    const vec4 vertexPosition_shadowViewSpace =
        i_viewProjectionData.data[gl_ViewIndex].viewTransform *
        vertexPosition_worldSpace;

    //
    // Output
    //
    o_vertexPosition_shadowViewSpace = vec3(vertexPosition_shadowViewSpace);
    o_instanceIndex = gl_InstanceIndex;
    o_fragTexCoord = i_vertexUv;

    // Final light-space position of this vertex.
    // Note that global data's view and projection transforms are set for
    // light-shadow-space conversion for this shader, not camera view-space.
    gl_Position =
        u_globalData.data.surfaceTransform *
        i_viewProjectionData.data[gl_ViewIndex].projectionTransform * // Light projection transform
        vertexPosition_shadowViewSpace;
}
