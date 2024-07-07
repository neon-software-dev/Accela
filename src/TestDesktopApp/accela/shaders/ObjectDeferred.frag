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

struct MaterialPayload
{
    bool isAffectedByLighting;

    vec4 ambientColor;
    vec4 diffuseColor;
    vec4 specularColor;
    uint alphaMode;
    float alphaCutoff;
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

const uint ALPHA_MODE_OPAQUE = 0;
const uint ALPHA_MODE_MASK = 1;
const uint ALPHA_MODE_BLEND = 2;

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
FragmentColors ProcessAlphaMode(MaterialPayload materialPayload, FragmentColors fragmentColors);
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
layout(location = 3) out uvec2 o_vertexObjectDetail;
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
    FragmentColors fragmentColors = CalculateFragmentColors(materialPayload);

    //
    // Transform the fragment colors by the material's alpha mode
    //
    fragmentColors = ProcessAlphaMode(materialPayload, fragmentColors);

    //
    // Discard fully transparent fragments.
    // (0.01f used instead of 0.0f to avoid potential precision issues.)
    //
    if (fragmentColors.ambientColor.a <= 0.01f &&
        fragmentColors.diffuseColor.a <= 0.01f &&
        fragmentColors.specularColor.a <= 0.01f)
    {
        discard;
    }

    //
    // Calculate the fragment's view-space normal for lighting shader to use
    //
    const vec3 fragmentNormal_modelSpace = CalculateFragmentModelNormal(materialPayload);
    const mat3 normalMVTransform = mat3(transpose(inverse(i_viewProjectionData.data[gl_ViewIndex].viewTransform * objectPayload.modelTransform)));
    const vec3 fragmentNormal_viewSpace = normalize(normalMVTransform * fragmentNormal_modelSpace);

    //
    // Write g-buffer outputs
    //
    o_fragColor = vec4(0, 0, 0, 1);
    o_vertexPosition_worldSpace = vec4(i_vertexPosition_worldSpace, 1);
    o_vertexNormal_viewSpace = vec4(fragmentNormal_viewSpace, 1);
    o_vertexObjectDetail.r = drawPayload.dataIndex + 1; // +1 converts from data index to object id
    o_vertexObjectDetail.g = drawPayload.materialIndex;
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
    fragColors.ambientColor = materialPayload.ambientColor;
    if (materialPayload.hasAmbientTexture)
    {
        vec4 textureColor = texture(i_ambientSampler, i_fragTexCoord) * materialPayload.ambientTextureBlendFactor;
        fragColors.ambientColor = TextureOp(fragColors.ambientColor, textureColor, materialPayload.ambientTextureOp);
    }

    // Diffuse
    fragColors.diffuseColor = materialPayload.diffuseColor;
    if (materialPayload.hasDiffuseTexture)
    {
        vec4 textureColor = texture(i_diffuseSampler, i_fragTexCoord) * materialPayload.diffuseTextureBlendFactor;
        fragColors.diffuseColor = TextureOp(fragColors.diffuseColor, textureColor, materialPayload.diffuseTextureOp);
    }

    // Specular
    fragColors.specularColor = materialPayload.specularColor;
    if (materialPayload.hasSpecularTexture)
    {
        vec4 textureColor = texture(i_specularSampler, i_fragTexCoord) * materialPayload.specularTextureBlendFactor;
        fragColors.specularColor = TextureOp(fragColors.specularColor, textureColor, materialPayload.specularTextureOp);
    }

    return fragColors;
}

FragmentColors ProcessAlphaMode(MaterialPayload materialPayload, FragmentColors fragmentColors)
{
    if (materialPayload.alphaMode == ALPHA_MODE_OPAQUE)
    {
        // "The rendered output is fully opaque and any alpha value is ignored."
        fragmentColors.ambientColor.a = 1.0f;
        fragmentColors.diffuseColor.a = 1.0f;
        fragmentColors.specularColor.a = 1.0f;
    }
    else if (materialPayload.alphaMode == ALPHA_MODE_MASK)
    {
        // "The rendered output is either fully opaque or fully transparent depending on the alpha value and
        // the specified alpha cutoff value."
        fragmentColors.ambientColor.a = fragmentColors.ambientColor.a >= materialPayload.alphaCutoff ? 1.0f : 0.0f;
        fragmentColors.diffuseColor.a = fragmentColors.diffuseColor.a >= materialPayload.alphaCutoff ? 1.0f : 0.0f;
        fragmentColors.specularColor.a = fragmentColors.specularColor.a >= materialPayload.alphaCutoff ? 1.0f : 0.0f;
    }
    else if (materialPayload.alphaMode == ALPHA_MODE_BLEND)
    {
        // no-op - use the alphas as specified by the material
    }

    return fragmentColors;
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
