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

struct DrawPayload
{
    uint dataIndex;
    uint materialIndex;
};

const uint SHADOW_MAP_TYPE_CASCADED = 0;        // Cascaded shadow map
const uint SHADOW_MAP_TYPE_SINGLE = 1;          // Single shadow map
const uint SHADOW_MAP_TYPE_CUBE = 2;            // Cubic shadow map

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

//
// Inputs
//
layout(location = 0) in vec3 i_vertexPosition_shadowViewSpace;
layout(location = 1) flat in int i_instanceIndex;
layout(location = 2) in vec2 i_fragTexCoord;

// Set 0 - Global Data
layout(set = 0, binding = 0) uniform GlobalPayloadUniform
{
    GlobalPayload data;
} u_globalData;

layout(set = 0, binding = 1) readonly buffer ViewProjectionPayloadUniform
{
    ViewProjectionPayload data[];
} i_viewProjectionData;

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

layout(push_constant) uniform constants
{
    uint shadowMapType;
    float lightMaxAffectRange;
} PushConstants;

void main()
{
    const DrawPayload drawPayload = i_drawData.data[i_instanceIndex];
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
    // Discard non-opaque fragments. We want only fully opaque fragments to
    // cast shadows.
    //
    if (fragmentColors.ambientColor.a < 0.99f &&
        fragmentColors.diffuseColor.a < 0.99f &&
        fragmentColors.specularColor.a < 0.99f)
    {
        discard;
    }

    float lightDistance = 0.0f;

    if (PushConstants.shadowMapType == SHADOW_MAP_TYPE_CASCADED)
    {
        const vec4 fragPosition_shadowClipSpace =
            u_globalData.data.surfaceTransform *
            i_viewProjectionData.data[gl_ViewIndex].projectionTransform *
            vec4(i_vertexPosition_shadowViewSpace, 1.0f);

        const vec3 fragPosition_shadowNDCSpace = fragPosition_shadowClipSpace.xyz / fragPosition_shadowClipSpace.w;

        // 0..1 within the scope of the shadow render orthograph projection depth
        lightDistance = fragPosition_shadowNDCSpace.z;
    }
    else
    {
        // Distance from the light to the vertex
        lightDistance = length(i_vertexPosition_shadowViewSpace);

        // 0..1 within the scope of the light's max affect range
        lightDistance = lightDistance / PushConstants.lightMaxAffectRange;
    }

    //
    // Output
    //
    gl_FragDepth = lightDistance;
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
