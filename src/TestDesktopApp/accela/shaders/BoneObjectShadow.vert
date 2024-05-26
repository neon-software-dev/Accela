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

struct BoneVertex
{
    vec4 pos_modelSpace;
    vec3 normal_modelSpace;
};

BoneVertex TransformVertexByBones();

//
// INPUTS
//

// Vertex Data
layout(location = 0) in vec3 i_vertexPosition_modelSpace;
layout(location = 1) in vec3 i_vertexNormal_modelSpace;
layout(location = 2) in vec2 i_vertexUv;
layout(location = 3) in vec3 i_vertexTangent_modelSpace;
layout(location = 5) in ivec4 i_bones;
layout(location = 6) in vec4 i_boneWeights;

// Set 0 - Global Data
layout(set = 0, binding = 0) uniform GlobalPayloadUniform
{
    GlobalPayload data;
} u_globalData;

layout(set = 0, binding = 1) readonly buffer ViewProjectionPayloadUniform
{
    ViewProjectionPayload data[];
} i_viewProjectionData;

// Set 1 - Renderer Data
layout(set = 1, binding = 0) readonly buffer ObjectPayloadBuffer
{
    ObjectPayload data[];
} i_objectData;

// Set 3 - Draw Data
layout(set = 3, binding = 0) readonly buffer DrawPayloadBuffer
{
    DrawPayload data[];
} i_drawData;

layout(set = 3, binding = 1) readonly buffer BonePayloadBuffer
{
    mat4 data[];
} i_boneData;

layout(set = 3, binding = 2) readonly buffer MeshPayloadBuffer
{
    uint numMeshBones;
} i_meshData;

// Push Constants
layout(push_constant) uniform constants
{
    float lightMaxAffectRange;
} PushConstants;

//
// OUTPUTS
//
layout(location = 0) out vec3 o_vertexPosition_shadowViewSpace;

void main()
{
    const DrawPayload drawPayload = i_drawData.data[gl_InstanceIndex];
    const ObjectPayload objectPayload = i_objectData.data[drawPayload.dataIndex];

    // Transform the vertex's details by the relevant object instance's bone transforms
    const BoneVertex boneVertex = TransformVertexByBones();

    const vec4 vertexPosition_worldSpace =
        objectPayload.modelTransform *
        boneVertex.pos_modelSpace;

    const vec4 vertexPosition_shadowViewSpace =
        i_viewProjectionData.data[gl_ViewIndex].viewTransform *
        vertexPosition_worldSpace;

    //
    // Output
    //
    o_vertexPosition_shadowViewSpace = vec3(vertexPosition_shadowViewSpace);

    // Final MVP position of this vertex
    gl_Position =
        u_globalData.data.surfaceTransform *
        i_viewProjectionData.data[gl_ViewIndex].projectionTransform *
        vertexPosition_shadowViewSpace;
}

// Keep this in sync with BoneObject.vert
BoneVertex TransformVertexByBones()
{
    BoneVertex boneVertex;
    boneVertex.pos_modelSpace = vec4(0);
    boneVertex.normal_modelSpace = vec3(0);

    // Get the index into the bone transforms vector which this instance's bone transforms are stored
    const uint numMeshBones = i_meshData.numMeshBones;
    const uint boneTransformsBaseOffset = gl_InstanceIndex * numMeshBones;

    // Each vertex can have up to 4 bones which modify it. Loop through all four and apply
    // their transformations together.
    for (int x = 0; x < 4; ++x)
    {
        // If a bone doesn't affect this vertex, do nothing
        if (i_bones[x] == -1) { continue; }

        const mat4 boneTransform = i_boneData.data[boneTransformsBaseOffset + i_bones[x]];

        // Modify the vertex's postion and normal by the amount the bone transform specifies
        boneVertex.pos_modelSpace += (boneTransform * vec4(i_vertexPosition_modelSpace, 1)) * i_boneWeights[x];
        boneVertex.normal_modelSpace += (mat3(boneTransform) * i_vertexNormal_modelSpace) * i_boneWeights[x];
    }

    boneVertex.normal_modelSpace = normalize(boneVertex.normal_modelSpace);

    return boneVertex;
}
