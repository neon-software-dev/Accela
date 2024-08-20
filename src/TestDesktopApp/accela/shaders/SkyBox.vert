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

//
// OUTPUTS
//
layout(location = 0) out vec3 o_fragTexCoord;           // The vertex's tex coord

void main()
{
    const vec4 vertexPosition =
        u_globalData.data.surfaceTransform *
        i_viewProjectionData.data[gl_ViewIndex].projectionTransform *
        i_viewProjectionData.data[gl_ViewIndex].viewTransform *
        vec4(i_vertexPosition_modelSpace, 1.0);

    // Puts the skybox vertices at full depth value so that everything else in the
    // scene can be drawn on top of the sky box.
    // See: https://learnopengl.com/Advanced-OpenGL/Cubemaps
    gl_Position = vertexPosition.xyww;

    // Outputs being passed to the fragment shader.
    // Note that the vertex position is used rather than the non-existent vertex uv
    o_fragTexCoord = i_vertexPosition_modelSpace;
    // Adjustment otherwise the sides are all flipped horizontally
    o_fragTexCoord.x *= -1.0;
}
