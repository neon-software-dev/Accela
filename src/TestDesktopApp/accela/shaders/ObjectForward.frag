/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#version 460
#extension GL_EXT_multiview : require
// #extension GL_EXT_debug_printf : enable

//
// Definitions
//
const uint Max_Light_Count = 16;                // Maximum number of scene lights
const uint Shadow_Cascade_Count = 4;            // Cascade count for cascaded shadow maps

const uint SHADOW_MAP_TYPE_CASCADED = 0;        // Cascaded shadow map
const uint SHADOW_MAP_TYPE_SINGLE = 1;          // Single shadow map
const uint SHADOW_MAP_TYPE_CUBE = 2;            // Cubic shadow map

const uint ATTENUATION_MODE_NONE = 0;           // Attenuation - none
const uint ATTENUATION_MODE_LINEAR = 1;         // Attenuation - linear decrease
const uint ATTENUATION_MODE_EXPONENTIAL = 2;    // Attenuation - exponential decrease

const uint ALPHA_MODE_OPAQUE = 0;               // Opaque final alpha
const uint ALPHA_MODE_MASK = 1;                 // Fully transparent or opaque final alpha depending on mask
const uint ALPHA_MODE_BLEND = 2;                // Translucent-capable

struct GlobalPayload
{
    // General
    mat4 surfaceTransform;          // Projection Space -> Rotated projection space

    // Lighting
    uint numLights;                 // Number of lights in the scene
    float ambientLightIntensity;    // Global ambient light intensity
    vec3 ambientLightColor;         // Global ambient light color
    float shadowCascadeOverlap;     // Ratio of overlap between cascade cuts
};

struct ViewProjectionPayload
{
    mat4 viewTransform;             // World Space -> View Space transform matrix
    mat4 projectionTransform;       // View Space -> Clip Space transform matrix
};

struct ShadowMapPayload
{
    vec3 worldPos;                  // World position the shadow map was rendered from
    mat4 transform;                 // View+Projection transform for the shadow map render

    // Directional shadow map specific
    vec2 cut;                       // Cascade [start, end] distances, in camera view space
    uint cascadeIndex;              // Cascade index [0..Shadow_Cascade_Count)
};

struct LightPayload
{
    vec3 worldPos;
    float maxAffectRange;

    // Base light properties
    uint attenuationMode;           // (ATTENUATION_MODE_{X})
    vec3 diffuseColor;
    vec3 diffuseIntensity;
    vec3 specularColor;
    vec3 specularIntensity;

    // Point light properties
    vec3 directionUnit;
    float areaOfEffect;

    // Shadow Map properties
    uint shadowMapType;             // SHADOW_MAP_TYPE_{X}
    int shadowMapIndex;             // Index into shadow map sampler for this light's shadow map(s)
    ShadowMapPayload shadowMaps[Shadow_Cascade_Count]; // Data about this light's shadow map renders
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

struct ObjectPayload
{
    mat4 modelTransform;
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

struct CalculatedLight
{
    vec3 ambientLight;
    vec3 diffuseLight;
    vec3 specularLight;
};

FragmentColors CalculateFragmentColors(MaterialPayload materialPayload);
FragmentColors ProcessAlphaMode(MaterialPayload materialPayload, FragmentColors fragmentColors);
vec3 CalculateFragmentModelNormal(MaterialPayload materialPayload);

CalculatedLight CalculateFragmentLighting(MaterialPayload fragmentMaterial, mat3 normalTransform, vec3 fragNormal_viewSpace, bool isTranslucent);
float GetFragShadowLevel(LightPayload lightData, vec3 fragPosition_viewSpace, vec3 fragPosition_worldSpace);
bool CanLightAffectFragment(LightPayload light, vec3 point_worldSpace);
ShadowMapPayload GetFragShadowMapPayload(LightPayload lightData, vec3 fragPosition_viewSpace);

// Offsets for (point-light) PCF filtering
const uint PCF_CUBIC_NUM_SAMPLES = 20;
const vec3 SampleOffsetDirections[PCF_CUBIC_NUM_SAMPLES] = vec3[]
(
    vec3(1, 1, 1), vec3(1, -1, 1), vec3(-1, -1, 1), vec3(-1, 1, 1),
    vec3(1, 1,-1), vec3(1,-1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
    vec3(1, 1, 0), vec3(1,-1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0),
    vec3(1, 0, 1), vec3(-1, 0, 1), vec3(1, 0, -1), vec3(-1, 0, -1),
    vec3(0, 1, 1), vec3(0, -1, 1), vec3(0, -1, -1), vec3(0, 1, -1)
);

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

layout(set = 0, binding = 2) readonly buffer LightPayloadBuffer
{
    LightPayload data[];
} i_lightData;

layout(set = 0, binding = 3) uniform sampler2DArray i_shadowSampler_cascaded[Max_Light_Count];
layout(set = 0, binding = 4) uniform sampler2D i_shadowSampler_single[Max_Light_Count];
layout(set = 0, binding = 5) uniform samplerCube i_shadowSampler_cube[Max_Light_Count];

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

layout(push_constant) uniform constants
{
    bool hdr;
} PushConstants;

//
// OUTPUTS
//
layout(location = 0) out vec4 o_fragColor;
layout(location = 1) out uvec2 o_vertexObjectDetail;

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

    // The fragment is translucent if any of its color components have a alpha between zero and one. Using
    // hundredths to account for float imprecision
    const bool isTranslucent =(
        (fragmentColors.ambientColor.a > 0.01f && fragmentColors.ambientColor.a < 0.99f) ||
        (fragmentColors.diffuseColor.a > 0.01f && fragmentColors.diffuseColor.a < 0.99f) ||
        (fragmentColors.specularColor.a > 0.01f && fragmentColors.specularColor.a < 0.99f)
    );

    //
    // Calculate the fragment's view-space normal for lighting to use
    //
    const vec3 fragmentNormal_modelSpace = CalculateFragmentModelNormal(materialPayload);
    const mat3 normalTransform = mat3(transpose(inverse(i_viewProjectionData.data[gl_ViewIndex].viewTransform * objectPayload.modelTransform)));
    const vec3 fragmentNormal_viewSpace = normalize(normalTransform * fragmentNormal_modelSpace);

    //
    // Calculate the light hitting the fragment
    //
    const CalculatedLight calculatedLight = CalculateFragmentLighting(materialPayload, normalTransform, fragmentNormal_viewSpace, isTranslucent);

    //
    // Combine the lighting with the fragment's color
    //
    const vec4 ambientColor = fragmentColors.ambientColor * vec4(calculatedLight.ambientLight, 1.0f);
    const vec4 diffuseColor = fragmentColors.diffuseColor * vec4(calculatedLight.diffuseLight, 1.0f);
    const vec4 specularColor = fragmentColors.specularColor * vec4(calculatedLight.specularLight, 1.0f);

    vec4 surfaceColor = ambientColor + diffuseColor + specularColor;

    // Set the alpha of the fragment to the fragment's max per-component alpha
    surfaceColor.a = max(ambientColor.a, diffuseColor.a);
    surfaceColor.a = max(surfaceColor.a, surfaceColor.a);

    //
    // If not doing HDR, clamp the brighest color between pure dark and pure bright
    //
    if (!PushConstants.hdr)
    {
        surfaceColor = clamp(surfaceColor, vec4(0, 0, 0, 0), vec4(1, 1, 1, 1));
    }

    o_fragColor = surfaceColor;
    o_vertexObjectDetail.r = drawPayload.dataIndex + 1; // +1 converts from data index to object id
    o_vertexObjectDetail.g = drawPayload.materialIndex;
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

CalculatedLight CalculateFragmentLighting(MaterialPayload fragmentMaterial, mat3 normalTransform, vec3 fragmentNormal_viewSpace, bool isTranslucent)
{
    const mat4 viewTransform = i_viewProjectionData.data[gl_ViewIndex].viewTransform;

    //
    // Calculate the light that's hitting the fragment from all the scene's lights
    //
    CalculatedLight light;
    light.ambientLight = u_globalData.data.ambientLightColor * u_globalData.data.ambientLightIntensity;
    light.diffuseLight = vec3(0, 0, 0);
    light.specularLight = vec3(0, 0, 0);

    // Get the position of the fragment
    const vec3 fragPosition_worldSpace = i_vertexPosition_worldSpace;
    const vec3 fragPosition_viewSpace = vec3(viewTransform * vec4(fragPosition_worldSpace, 1.0f));

    // Position of the camera, in view space
    const vec3 cameraPosition_viewSpace = vec3(0, 0, 0);

    // Unit vector which points from the fragment's position to the camera's position
    vec3 fragToCameraDirUnit_viewSpace = normalize(cameraPosition_viewSpace - fragPosition_viewSpace);

    // Process potential lighting additions from each light in the scene
    for (uint x = 0; x < u_globalData.data.numLights; ++x)
    {
        const LightPayload lightData = i_lightData.data[x];

        // If the light can't reach/touch the fragment, there's no contributation from it, ignore it
        if (!CanLightAffectFragment(lightData, fragPosition_worldSpace))
        {
            continue;
        }

        // Position of the light, in view space
        const vec3 lightPos_viewSpace = vec3(viewTransform * vec4(lightData.worldPos, 1));

        // Calculate the unit vector which points from the fragment's position to the light, and the
        // distance from the fragment to the light
        vec3 fragTolightUnit_viewSpace = vec3(0,0,0);
        float fragToLightDistance = 0.0f;

        if (lightData.shadowMapType == SHADOW_MAP_TYPE_CASCADED)
        {
            // Directional lights are considered to be infinitely far away with parallel rays, and thus
            // we ignore the position the light is at and just use the opposite of the light's direction
            // as the direction to the light. Also applying the normal matrix to convert the direction
            // vector from world space to view space, so that translation/scale in the view transform
            // doesn't impact the direction vector, only camera rotation.
            fragTolightUnit_viewSpace = normalize(normalTransform * -lightData.directionUnit);

            // However, we DO use the light's position for attenuation purposes. Note that the typical
            // denominator in the ray/plane intersection is 1 in this case, and so is ignored
            fragToLightDistance = dot((lightData.worldPos - fragPosition_worldSpace), -lightData.directionUnit);
        }
        else if (lightData.shadowMapType == SHADOW_MAP_TYPE_SINGLE || lightData.shadowMapType == SHADOW_MAP_TYPE_CUBE)
        {
            // Point and spot lights eminate from a specific world position, so we can draw a vector from the
            // fragment to the light's position
            fragTolightUnit_viewSpace = normalize(lightPos_viewSpace - fragPosition_viewSpace);

            // Physical world distance between the fragment and the light
            fragToLightDistance = distance(lightPos_viewSpace, fragPosition_viewSpace);
        }

        // Shouldn't ever be the case with non-zero perspective near planes
        if (fragToLightDistance == 0.0f) {  continue; }

        // Calculate whether the fragment is in shadow from the light
        const float fragShadowLevel = isTranslucent ? 0.0f : GetFragShadowLevel(lightData, fragPosition_viewSpace, fragPosition_worldSpace);

        // If the fragment is in full shadow from the light, the light doesn't affect it, bail out early
        if (fragShadowLevel == 1.0f)
        {
            continue;
        }

        // Calculate light attenuation
        float lightAttenuation = 1.0f;

        if (lightData.attenuationMode == ATTENUATION_MODE_NONE)
        {
            lightAttenuation = 1.0f;
        }
        else if (lightData.attenuationMode == ATTENUATION_MODE_LINEAR)
        {
            // c1 / d with c1 = 10.0
            // Note: Don't change the constant(s) without updating relevant lighting code
            lightAttenuation = clamp(10.0f / fragToLightDistance, 0.0f, 1.0f);
        }
        else if (lightData.attenuationMode == ATTENUATION_MODE_EXPONENTIAL)
        {
            // 1.0 / (c1 + c2*d^2) with c1 = 1.0, c2 = 0.1
            // Note: Don't change the constant(s) without updating relevant lighting code
            lightAttenuation = 1.0/(1.0f + 0.1f * pow(fragToLightDistance, 2));
        }

        //
        // Diffuse Light
        //
        const vec3 vertexNormal_viewSpace = fragmentNormal_viewSpace;

        const float diffuseFragHitPercentage = clamp(
            dot(vertexNormal_viewSpace, fragTolightUnit_viewSpace),
            0, 1
        );

        // Add in the diffuse light contribution from this light source
        light.diffuseLight +=   lightData.diffuseColor *
                                lightData.diffuseIntensity *
                                diffuseFragHitPercentage *
                                lightAttenuation *
                                (1.0f - fragShadowLevel);

        //
        // Specular light
        //
        const float shininess = fragmentMaterial.shininess;

        if (shininess != 0)
        {
            // Verify that the fragment points at the light
            if (dot(fragTolightUnit_viewSpace, vertexNormal_viewSpace) > 0.0f)
            {
                // Unit vector which is the reflection of the light off the surface
                const vec3 fragLightReflectDirUnit_viewSpace = normalize(reflect(-fragTolightUnit_viewSpace, vertexNormal_viewSpace));

                // Blinn-Phong half-way vector calculation; half-way between frag to light and frag to camera vectors
                const vec3 halfwayUnit_viewSpace = normalize(fragTolightUnit_viewSpace + fragToCameraDirUnit_viewSpace);
                const float specularFragHitPercentage = max(dot(vertexNormal_viewSpace, halfwayUnit_viewSpace), 0.0f);

                const float spec =  pow(specularFragHitPercentage, shininess);

                light.specularLight +=  lightData.specularColor *
                                        lightData.specularIntensity *
                                        spec *
                                        lightAttenuation *
                                        (1.0f - fragShadowLevel);
            }
        }
    }

    // If not in HDR mode, Clamp RGB values to full intensity if there's too much light
    if (!PushConstants.hdr)
    {
        light.ambientLight = clamp(light.ambientLight, vec3(0, 0, 0), vec3(1, 1, 1));
        light.diffuseLight = clamp(light.diffuseLight, vec3(0, 0, 0), vec3(1, 1, 1));
        light.specularLight = clamp(light.specularLight, vec3(0, 0, 0), vec3(1, 1, 1));
    }

    return light;
}

uint GetFragCascadeIndex(LightPayload lightData, vec3 fragPosition_viewSpace)
{
    // Z-distance along the camera view projection
    const float fragDistance_viewSpace = abs(fragPosition_viewSpace.z);

    for (uint cascadeIndex = 0; cascadeIndex < Shadow_Cascade_Count; ++cascadeIndex)
    {
        if (fragDistance_viewSpace <= lightData.shadowMaps[cascadeIndex].cut.y)
        {
            return cascadeIndex;
        }
    }

    // Shouldn't ever be the case
    return Shadow_Cascade_Count - 1;
}

ShadowMapPayload GetFragShadowMapPayload(LightPayload lightData, vec3 fragPosition_viewSpace)
{
    if (lightData.shadowMapType == SHADOW_MAP_TYPE_CASCADED)
    {
        // Return the shadow map render for a specific cascade, dependent on frag position
        return lightData.shadowMaps[GetFragCascadeIndex(lightData, fragPosition_viewSpace)];
    }
    else
    {
        // Single and cube shadow maps only have one shadow map render
        return lightData.shadowMaps[0];
    }
}

float GetFragShadowLevel_Cascaded(LightPayload lightData, ShadowMapPayload shadowMap, vec3 fragPosition_worldSpace, float lightToFragDepth)
{
    const vec4 fragPosition_lightClipSpace = shadowMap.transform * vec4(fragPosition_worldSpace, 1);

    // Sanity check the fragment is within the shadow map, although this method should never be called if that's the case
    if ((abs(fragPosition_lightClipSpace.x) > fragPosition_lightClipSpace.w) ||
        (abs(fragPosition_lightClipSpace.y) > fragPosition_lightClipSpace.w) ||
        (fragPosition_lightClipSpace.z > fragPosition_lightClipSpace.w) ||
        (fragPosition_lightClipSpace.z < 0.0f))
    {
        return 0.0f;
    }

    const vec3 fragPosition_lightNDCSpace = fragPosition_lightClipSpace.xyz / fragPosition_lightClipSpace.w;
    const vec2 shadowSampleCoords = fragPosition_lightNDCSpace.xy * 0.5f + 0.5f;

    const vec2 texelSize = 1.0 / textureSize(i_shadowSampler_cascaded[lightData.shadowMapIndex], 0).xy;

    float shadowLevel = 0.0;
    int sampleSize = 1;

    // PCF filter
    for (int x = -sampleSize; x <= sampleSize; ++x)
    {
        for (int y = -sampleSize; y <= sampleSize; ++y)
        {
            const float pcfFragDepth = texture(
                i_shadowSampler_cascaded[lightData.shadowMapIndex],
                vec3(shadowSampleCoords + (vec2(x, y) * texelSize), shadowMap.cascadeIndex)
            ).r;

            shadowLevel += lightToFragDepth > pcfFragDepth ? 1.0 : 0.0;
        }
    }

    return shadowLevel / ((sampleSize * 2.0f + 1.0f) * 2.0f);
}

float GetFragShadowLevel_Cascaded(LightPayload lightData, vec3 fragPosition_viewSpace, vec3 fragPosition_worldSpace)
{
    const float fragDistance_viewSpace = abs(fragPosition_viewSpace.z);

    //
    // Get the shadow level from the cascade the fragment falls within
    //
    ShadowMapPayload shadowMap = GetFragShadowMapPayload(lightData, fragPosition_viewSpace);
    const float cutRange = shadowMap.cut.y - shadowMap.cut.x;
    const float cutBlendRange = cutRange * u_globalData.data.shadowCascadeOverlap;
    const float cutBlendStart = shadowMap.cut.y - cutBlendRange;

    const bool fragWithinCutBlendBand = (fragDistance_viewSpace >= cutBlendStart) &&
    (shadowMap.cascadeIndex < (Shadow_Cascade_Count - 1));

    vec4 fragPosition_lightClipSpace = shadowMap.transform * vec4(fragPosition_worldSpace, 1);
    vec3 fragPosition_lightNDCSpace = fragPosition_lightClipSpace.xyz / fragPosition_lightClipSpace.w;
    float lightToFragDepth = fragPosition_lightNDCSpace.z;

    const float fragShadowLevelMain = GetFragShadowLevel_Cascaded(lightData, shadowMap, fragPosition_worldSpace, lightToFragDepth);

    //
    // If the fragment doesn't fall within the cut blend band, return the shadow level as-is
    //
    if (!fragWithinCutBlendBand)
    {
        return fragShadowLevelMain;
    }

    //
    // Otherwise, get the shadow level from the next cascade as well, and blend the two shadow levels together
    //
    shadowMap = lightData.shadowMaps[shadowMap.cascadeIndex + 1];
    fragPosition_lightClipSpace = shadowMap.transform * vec4(fragPosition_worldSpace, 1);
    fragPosition_lightNDCSpace = fragPosition_lightClipSpace.xyz / fragPosition_lightClipSpace.w;
    lightToFragDepth = fragPosition_lightNDCSpace.z;

    const float fragShadowLevelNext = GetFragShadowLevel_Cascaded(lightData, shadowMap, fragPosition_worldSpace, lightToFragDepth);

    //
    // Blend the two levels together depending on progress through the blend band
    //
    const float percentWithinBlendBand = (fragDistance_viewSpace - cutBlendStart) / cutBlendRange;

    return (fragShadowLevelMain * (1.0f - percentWithinBlendBand)) + (fragShadowLevelNext * percentWithinBlendBand);
}

float GetFragShadowLevel_Single(LightPayload lightData, vec3 fragPosition_viewSpace, vec3 fragPosition_worldSpace)
{
    //
    // Calculate depth from the light to the fragment
    //

    const vec3 lightToFrag = fragPosition_worldSpace - lightData.worldPos;
    const float lightToFragDepth = length(lightToFrag) / lightData.maxAffectRange;

    //
    // Query the shadow map for the closest fragment depth
    //

    const ShadowMapPayload shadowMap = GetFragShadowMapPayload(lightData, fragPosition_viewSpace);

    const vec4 fragPosition_lightClipSpace = shadowMap.transform * vec4(fragPosition_worldSpace, 1);

    // Sanity check the fragment is within the shadow map, although this method should never be called if that's the case
    if ((abs(fragPosition_lightClipSpace.x) > fragPosition_lightClipSpace.w) ||
        (abs(fragPosition_lightClipSpace.y) > fragPosition_lightClipSpace.w) ||
        (fragPosition_lightClipSpace.z > fragPosition_lightClipSpace.w) ||
        (fragPosition_lightClipSpace.z < 0.0f))
    {
        return 0.0f;
    }

    const vec3 fragPosition_lightNDCSpace = fragPosition_lightClipSpace.xyz / fragPosition_lightClipSpace.w;
    const vec2 shadowSampleCoords = fragPosition_lightNDCSpace.xy * 0.5f + 0.5f;

    const vec2 texelSize = 1.0 / textureSize(i_shadowSampler_single[lightData.shadowMapIndex], 0).xy;

    float shadow = 0.0;
    int sampleSize = 1;

    // PCF filter
    for (int x = -sampleSize; x <= sampleSize; ++x)
    {
        for (int y = -sampleSize; y <= sampleSize; ++y)
        {
            const float pcfFragDepth = texture(
            i_shadowSampler_single[lightData.shadowMapIndex],
            shadowSampleCoords + (vec2(x, y) * texelSize)
            ).r;

            shadow += lightToFragDepth > pcfFragDepth ? 1.0 : 0.0;
        }
    }

    return shadow / ((sampleSize * 2.0f + 1.0f) * 2.0f);
}

float GetFragShadowLevel_Cube(LightPayload lightData, vec3 fragPosition_worldSpace)
{
    //
    // Calculate depth from the light to the fragment
    //

    const vec3 lightToFrag = fragPosition_worldSpace - lightData.worldPos;
    const float lightToFragDepth = length(lightToFrag) / lightData.maxAffectRange;

    //
    // Query the shadow map for the closest fragment depth
    //

    // Vector from the light to the fragment, used when sampling the light's shadow cube map
    vec3 lightToFrag_worldSpace = fragPosition_worldSpace - lightData.worldPos;

    // Convert to cube-map left-handed coordinate system with swapped z-axis
    lightToFrag_worldSpace.z *= -1.0f;

    // Offsets for PCF samples
    const uint numSamples = PCF_CUBIC_NUM_SAMPLES;
    const float diskRadius = (1.0 + (length(lightToFrag_worldSpace) / lightData.maxAffectRange)) / 100.0;

    float shadow = 0.0;

    for (uint i = 0; i < numSamples; ++i)
    {
        const float pcfFragDepth = texture(
            i_shadowSampler_cube[lightData.shadowMapIndex],
            lightToFrag_worldSpace + (SampleOffsetDirections[i] * diskRadius)
        ).r;

        shadow += lightToFragDepth > pcfFragDepth ? 1.0 : 0.0;
    }

    return shadow / float(numSamples);
}

float GetFragShadowLevel(LightPayload lightData, vec3 fragPosition_viewSpace, vec3 fragPosition_worldSpace)
{
    // If the light has no shadow map, then the fragment isn't in shadow from it
    if (lightData.shadowMapIndex == -1) {  return 0.0f;  }

    switch (lightData.shadowMapType)
    {
        case SHADOW_MAP_TYPE_CASCADED:
        {
            return GetFragShadowLevel_Cascaded(lightData, fragPosition_viewSpace, fragPosition_worldSpace);
        }

        case SHADOW_MAP_TYPE_SINGLE:
        {
            return GetFragShadowLevel_Single(lightData, fragPosition_viewSpace, fragPosition_worldSpace);
        }

        case SHADOW_MAP_TYPE_CUBE:
        {
            return GetFragShadowLevel_Cube(lightData, fragPosition_worldSpace);
        }

        // Unsupported light type, return no shadow since we don't know how to access its shadow map
        default: {  return 0.0f; }
    }
}

bool CanLightAffectFragment_Directional(LightPayload light, vec3 fragPosition_worldSpace)
{
    //
    // Verify the fragment is within range of the light
    //
    const float fragToLightPlaneDistance = dot((light.worldPos - fragPosition_worldSpace), -light.directionUnit);

    if (light.attenuationMode != ATTENUATION_MODE_NONE)
    {
        if (abs(fragToLightPlaneDistance) > light.maxAffectRange)
        {
            return false;
        }
    }

    //
    // Verify the fragment falls within the light's emit disk
    //

    // If the fragment is behind the light, it's not affected by it
    if (fragToLightPlaneDistance < 0.0f)
    {
        return false;
    }

    // If the light doesn't use a specific area of effect, then it affects everything in front of it
    if (light.areaOfEffect <= 0.0001f)
    {
        return true;
    }

    // Otherwise, do a ray/disk intersection test to see whether the fragment is within the disk on
    // the light plane that light is being emitted from
    const vec3 intersectionPoint = fragPosition_worldSpace + (-light.directionUnit * fragToLightPlaneDistance);
    const float radius = distance(intersectionPoint, light.worldPos);

    // Outside the area of effect disk
    if (radius > light.areaOfEffect)
    {
        return false;
    }

    return true;
}

bool CanLightAffectFragment_Point(LightPayload light, vec3 fragPosition_worldSpace)
{
    //
    // Verify the fragment is within range of the light
    //
    const vec3 lightToPoint_worldSpaceUnit = normalize(fragPosition_worldSpace - light.worldPos);

    if (light.attenuationMode != ATTENUATION_MODE_NONE)
    {
        if (length(lightToPoint_worldSpaceUnit) > light.maxAffectRange)
        {
            return false;
        }
    }

    //
    // Verify the fragment falls within the light's cone fov
    //
    const float vectorAlignment = dot(light.directionUnit, lightToPoint_worldSpaceUnit);
    const float alignmentAngleDegrees = degrees(acos(vectorAlignment));
    const bool alignedWithLightCone = alignmentAngleDegrees <= light.areaOfEffect / 2.0f;

    if (!alignedWithLightCone)
    {
        return false;
    }

    return true;
}

bool CanLightAffectFragment(LightPayload light, vec3 fragPosition_worldSpace)
{
    if (light.shadowMapType == SHADOW_MAP_TYPE_CASCADED)
    {
        return CanLightAffectFragment_Directional(light, fragPosition_worldSpace);
    }
    else
    {
        return CanLightAffectFragment_Point(light, fragPosition_worldSpace);
    }
}
