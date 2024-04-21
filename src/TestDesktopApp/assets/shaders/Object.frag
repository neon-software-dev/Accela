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

struct ObjectPayload
{
    mat4 modelTransform;
};

struct MaterialPayload
{
    bool isAffectedByLighting;

    vec3 ambientColor;
    vec3 diffuseColor;
    vec3 specularColor;
    float opacity;
    float shininess;

    bool hasAmbientTexture;
    float ambientTextureBlendFactor;
    uint ambientTextureOp;

    bool hasDiffuseTexture;
    float diffuseTextureBlendFactor;
    uint diffuseTextureOp;

    bool hasSpecularTexture;
    float specularTextureBlendFactor;
    uint specularTextureOp;

    bool hasNormalTexture;
};

//
// Internal
//
struct FragmentColors
{
    vec4 ambientColor;
    vec4 diffuseColor;
    vec4 specularColor;
};

FragmentColors CalculateFragmentColors(MaterialPayload materialPayload);
vec3 CalculateFragmentModelNormal(MaterialPayload materialPayload);

//
// INPUTS
//
layout(location = 0) flat in int i_instanceIndex;
layout(location = 1) in vec2 i_fragTexCoord;
layout(location = 2) in vec3 i_vertexNormal_modelSpace;
layout(location = 3) in vec3 i_vertexPosition_worldSpace;
layout(location = 4) in mat3 i_tbnNormalTransform;

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

// Set 2 - Material Data
layout(set = 2, binding = 0) readonly buffer MaterialPayloadBuffer
{
    MaterialPayload data[];
} i_materialData;

layout(set = 2, binding = 1) uniform sampler2D i_ambientSampler;
layout(set = 2, binding = 2) uniform sampler2D i_diffuseSampler;
layout(set = 2, binding = 3) uniform sampler2D i_specularSampler;
layout(set = 2, binding = 4) uniform sampler2D i_normalSampler;

// Set 3 - Draw Data
layout(set = 3, binding = 0) readonly buffer DrawPayloadBuffer
{
    DrawPayload data[];
} i_drawData;

//
// OUTPUTS
//
layout(location = 0) out vec4 o_fragColor; // Is set to a default black value here and overwritten by the deferred lighting shader later
layout(location = 1) out vec4 o_vertexPosition_worldSpace;
layout(location = 2) out vec4 o_vertexNormal_viewSpace;
layout(location = 3) out uint o_vertexMaterial;
layout(location = 4) out vec4 o_vertexAmbientColor;
layout(location = 5) out vec4 o_vertexDiffuseColor;
layout(location = 6) out vec4 o_vertexSpecularColor;

void main()
{
    const DrawPayload drawPayload = i_drawData.data[i_instanceIndex];
    const ObjectPayload objectPayload = i_objectData.data[drawPayload.dataIndex];
    const MaterialPayload materialPayload = i_materialData.data[drawPayload.materialIndex];

    //
    // Determine the fragment's colors as reported by the model's material
    //
    const FragmentColors fragmentColors = CalculateFragmentColors(materialPayload);

    // To help with transparency, discard fragments that are sufficiently close to fully transparent,
    // so that they don't override what might have already been behind them in the depth buffer with
    // a transparent pixel
    // TODO: Need to add a separate forward pass for objects with any transparency, after the deferred pass
    if (fragmentColors.diffuseColor.a < 0.001f) {  discard;  }

    const vec3 fragmentNormal_modelSpace = CalculateFragmentModelNormal(materialPayload);
    const mat3 normalMVTransform = mat3(transpose(inverse(i_viewProjectionData.data[gl_ViewIndex].viewTransform * objectPayload.modelTransform)));
    const vec3 fragmentNormal_viewSpace = normalize(normalMVTransform * fragmentNormal_modelSpace);

    //
    // Write g-buffer outputs
    //
    o_fragColor = vec4(0, 0, 0, 1);
    o_vertexPosition_worldSpace = vec4(i_vertexPosition_worldSpace, 1);
    o_vertexNormal_viewSpace = vec4(fragmentNormal_viewSpace, 1);
    o_vertexMaterial = drawPayload.materialIndex;
    o_vertexAmbientColor = fragmentColors.ambientColor;
    o_vertexDiffuseColor = fragmentColors.diffuseColor;
    o_vertexSpecularColor = fragmentColors.specularColor;
}

vec4 TextureOp(vec4 dest, vec4 texture, uint textureOp)
{
    if (textureOp == 0)         { return dest * texture; }
    else if (textureOp == 1)    { return dest + texture; }
    else if (textureOp == 2)    { return dest - texture; }
    else if (textureOp == 3)    { return dest / texture; }
    else if (textureOp == 4)    { return (dest + texture) - (dest * texture); }
    else if (textureOp == 5)    { return dest + (texture - 0.5f); }
    else                        { return dest; }
}

FragmentColors CalculateFragmentColors(MaterialPayload materialPayload)
{
    FragmentColors fragColors;

    // Ambient
    fragColors.ambientColor = vec4(materialPayload.ambientColor, 1);
    if (materialPayload.hasAmbientTexture)
    {
        vec4 textureColor = texture(i_ambientSampler, i_fragTexCoord) * materialPayload.ambientTextureBlendFactor;
        // WARNING! NOTE! We're forcing the texture op to addition below, only for ambient color, because all of our
        // test models keep erroneously providing an ambient texture but a black ambient color. Forcing addition
        // as the default for ambient for the moment just to make testing easier without having to fix the models.
        // To fix, replace the 1 below with materialPayload.ambientTextureOp .
        fragColors.ambientColor = TextureOp(fragColors.ambientColor, textureColor, 1);
    }

    // Diffuse
    fragColors.diffuseColor = vec4(materialPayload.diffuseColor, 1);
    if (materialPayload.hasDiffuseTexture)
    {
        vec4 textureColor = texture(i_diffuseSampler, i_fragTexCoord) * materialPayload.diffuseTextureBlendFactor;
        fragColors.diffuseColor = TextureOp(fragColors.diffuseColor, textureColor, materialPayload.diffuseTextureOp);
    }

    // Specular
    fragColors.specularColor = vec4(materialPayload.specularColor, 1);
    if (materialPayload.hasSpecularTexture)
    {
        vec4 textureColor = texture(i_specularSampler, i_fragTexCoord) * materialPayload.specularTextureBlendFactor;
        fragColors.specularColor = TextureOp(fragColors.specularColor, textureColor, materialPayload.specularTextureOp);
    }

    return fragColors;
}

vec3 CalculateFragmentModelNormal(MaterialPayload materialPayload)
{
    vec3 modelNormal = i_vertexNormal_modelSpace;

    if (materialPayload.hasNormalTexture)
    {
        // Read the fragment normal from the normal map, and use the tbn matrix to convert it from
        // tangent space to model space
        modelNormal = normalize(i_tbnNormalTransform * texture(i_normalSampler, i_fragTexCoord).rgb);
    }

    return modelNormal;
}
