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

struct LightPayload
{
    uint shadowMapType;
    mat4 lightTransform;
    vec3 worldPos;
    int shadowMapIndex;
    float maxAffectRange;

    // Base properties
    uint attenuationMode;
    vec3 diffuseColor;
    vec3 diffuseIntensity;
    vec3 specularColor;
    vec3 specularIntensity;

    // Point light properties
    vec3 directionUnit;
    float coneFovDegrees;
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

struct CalculatedLight
{
    vec3 ambientLight;
    vec3 diffuseLight;
    vec3 specularLight;
};

const uint Max_Light_Count = 16;

const uint SHADOW_MAP_TYPE_SINGLE = 0;
const uint SHADOW_MAP_TYPE_CUBE = 1;

FragmentColors CalculateFragmentColors(MaterialPayload materialPayload);
FragmentColors ProcessAlphaMode(MaterialPayload materialPayload, FragmentColors fragmentColors);
vec3 CalculateFragmentModelNormal(MaterialPayload materialPayload);

CalculatedLight CalculateFragmentLighting(MaterialPayload fragmentMaterial, vec3 fragmentNormal_viewSpace);
float GetFragShadowLevel(LightPayload lightData, vec3 fragPosition_worldSpace);
float GetLightFragDepth(LightPayload lightData, vec3 fragPosition_worldSpace);
float GetFragShadowLevel_Single(LightPayload lightData, vec3 fragPosition_worldSpace, float lightToFragDepth);
float GetFragShadowLevel_Cube(LightPayload lightData, vec3 fragPosition_worldSpace, float lightToFragDepth);
bool IsPointWithinLightCone(LightPayload light, vec3 point_worldSpace);

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

layout(set = 0, binding = 3) uniform sampler2D i_shadowSampler[Max_Light_Count]; // Samplers for spot lights
layout(set = 0, binding = 4) uniform samplerCube i_shadowSampler_cubeMap[Max_Light_Count]; // Samplers for point lights

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
layout(location = 0) out vec4 o_fragColor;

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
    // Calculate the fragment's view-space normal for lighting to use
    //
    const vec3 fragmentNormal_modelSpace = CalculateFragmentModelNormal(materialPayload);
    const mat3 normalMVTransform = mat3(transpose(inverse(i_viewProjectionData.data[gl_ViewIndex].viewTransform * objectPayload.modelTransform)));
    const vec3 fragmentNormal_viewSpace = normalize(normalMVTransform * fragmentNormal_modelSpace);

    //
    // Calculate the light hitting the fragment
    //
    const CalculatedLight calculatedLight = CalculateFragmentLighting(materialPayload, fragmentNormal_viewSpace);

    //
    // Combine the lighting with the fragment's color
    //
    const vec4 ambientColor = fragmentColors.ambientColor * vec4(calculatedLight.ambientLight, 1.0f);
    const vec4 diffuseColor = fragmentColors.diffuseColor * vec4(calculatedLight.diffuseLight, 1.0f);
    const vec4 specularColor = fragmentColors.specularColor * vec4(calculatedLight.specularLight, 1.0f);

    vec4 surfaceColor = ambientColor + diffuseColor + specularColor;

    //
    // Clamp the brighest color between pure dark and pure bright
    //
    surfaceColor = clamp(surfaceColor, vec4(0, 0, 0, 0), vec4(1, 1, 1, 1));

    o_fragColor = surfaceColor;
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

CalculatedLight CalculateFragmentLighting(MaterialPayload fragmentMaterial, vec3 fragmentNormal_viewSpace)
{
    const mat4 viewTransform = i_viewProjectionData.data[gl_ViewIndex].viewTransform;

    CalculatedLight light;

    //
    // If the fragment's material isn't affected by lighting, then just return fully
    // lit values for all lighting parameters
    // TODO
    /*if (!materialPayload.isAffectedByLighting)
    {
        light.ambientLight = vec3(1, 1, 1);
        light.diffuseLight = vec3(1, 1, 1);
        light.specularLight = vec3(1, 1, 1);
        return light;
    }*/

    //
    // Otherwise, calculate what light is hitting the fragment
    //
    light.ambientLight = u_globalData.data.ambientLightColor * u_globalData.data.ambientLightIntensity;
    light.diffuseLight = vec3(0, 0, 0);
    light.specularLight = vec3(0, 0, 0);

    // Get the position of the fragment, in view space
    const vec3 fragPosition_worldSpace = i_vertexPosition_worldSpace;
    const vec3 fragPosition_viewSpace = vec3(viewTransform * vec4(fragPosition_worldSpace, 1.0f));

    // Position of the camera, in view space
    const vec3 cameraPosition_viewSpace = vec3(0, 0, 0);

    // Unit vector which points from the fragment's position to the camera's position
    vec3 fragToCameraDirUnit_viewSpace = normalize(cameraPosition_viewSpace - fragPosition_viewSpace);

    for (uint x = 0; x < u_globalData.data.numLights; ++x)
    {
        const LightPayload lightData = i_lightData.data[x];

        // Position of the light, in view space
        const vec3 lightPos_viewSpace = vec3(viewTransform * vec4(lightData.worldPos, 1));

        // Unit vector which points from the fragment's position to the light's position
        const vec3 fragTolightUnit_viewSpace = normalize(lightPos_viewSpace - fragPosition_viewSpace);

        // Distance between the light position and fragment position
        const float fragToLightDistance = distance(lightPos_viewSpace, fragPosition_viewSpace);

        // Shouldn't ever be the case with non-zero perspective near planes
        if (fragToLightDistance == 0.0f) {  continue; }

        // If the fragment is outside the light's cone, the light obviously doesn't hit it, ignore it
        const bool fragmentWithinLightCone = IsPointWithinLightCone(lightData, fragPosition_worldSpace);
        if (!fragmentWithinLightCone)
        {
            continue;
        }

        const float fragShadowLevel = GetFragShadowLevel(lightData, fragPosition_worldSpace);

        // If the fragment is in full shadow from the light, the light doesn't hit it, bail out early
        if (fragShadowLevel == 1.0f)
        {
            continue;
        }

        // Calculate light attenuation
        float lightAttenuation = 1.0f;

        if (lightData.attenuationMode == 0) // No attenuation
        {
            lightAttenuation = 1.0f;
        }
        else if (lightData.attenuationMode == 1) // Linear attenuation
        {
            // c1 / d with c1 = 10.0
            // Note: Don't change the constant(s) without updating relevant lighting code
            lightAttenuation = clamp(10.0f / fragToLightDistance, 0.0f, 1.0f);
        }
        else if (lightData.attenuationMode == 2) // Exponential attenuation
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

    // Clamp RGB values to full intensity if there's too much light
    light.ambientLight = clamp(light.ambientLight, vec3(0, 0, 0), vec3(1, 1, 1));
    light.diffuseLight = clamp(light.diffuseLight, vec3(0, 0, 0), vec3(1, 1, 1));
    light.specularLight = clamp(light.specularLight, vec3(0, 0, 0), vec3(1, 1, 1));

    return light;
}

float GetFragShadowLevel(LightPayload lightData, vec3 fragPosition_worldSpace)
{
    // If the light has no shadow map, then the fragment isn't in shadow from it
    if (lightData.shadowMapIndex == -1) {  return 0.0f;  }

    // Depth distance [0,1] from the light to the provided fragment's position
    const float lightToFragDepth = GetLightFragDepth(lightData, fragPosition_worldSpace);

    switch (lightData.shadowMapType)
    {
        case SHADOW_MAP_TYPE_SINGLE:
        return GetFragShadowLevel_Single(lightData, fragPosition_worldSpace, lightToFragDepth);

        case SHADOW_MAP_TYPE_CUBE:
        return GetFragShadowLevel_Cube(lightData, fragPosition_worldSpace, lightToFragDepth);

        // Unsupported light type, return no shadow since we don't know how to access its shadow map
        default: {  return 0.0f; }
    }
}

float GetLightFragDepth(LightPayload lightData, vec3 fragPosition_worldSpace)
{
    const vec3 lightToFrag = fragPosition_worldSpace - lightData.worldPos;

    return length(lightToFrag) / lightData.maxAffectRange;
}

float GetFragShadowLevel_Single(LightPayload lightData, vec3 fragPosition_worldSpace, float lightToFragDepth)
{
    const vec4 fragPosition_lightClipSpace = lightData.lightTransform * vec4(fragPosition_worldSpace, 1);
    const vec3 fragPosition_lightNDCSpace = fragPosition_lightClipSpace.xyz / fragPosition_lightClipSpace.w;
    const vec2 shadowSampleCoords = fragPosition_lightNDCSpace.xy * 0.5f + 0.5f;

    const vec2 texelSize = 1.0 / textureSize(i_shadowSampler[lightData.shadowMapIndex], 0);

    float shadow = 0.0;
    int sampleSize = 1;

    // PCF filter
    for (int x = -sampleSize; x <= sampleSize; ++x)
    {
        for (int y = -sampleSize; y <= sampleSize; ++y)
        {
            const float pcfFragDepth = texture(
            i_shadowSampler[lightData.shadowMapIndex],
            shadowSampleCoords + (vec2(x, y) * texelSize)
            ).r;

            shadow += lightToFragDepth > pcfFragDepth ? 1.0 : 0.0;
        }
    }

    return shadow / ((sampleSize * 2.0f + 1.0f) * 2.0f);
}

float GetFragShadowLevel_Cube(LightPayload lightData, vec3 fragPosition_worldSpace, float lightToFragDepth)
{
    // Vector from the light to the fragment, used when sampling the light's shadow cube map
    vec3 lightToFrag_worldSpace = fragPosition_worldSpace - lightData.worldPos;

    // Correct for cube map coordinate system and sampling rules differing from our internal coordinate system
    const float xAbs = abs(lightToFrag_worldSpace.x);
    const float yAbs = abs(lightToFrag_worldSpace.y);
    const float zAbs = abs(lightToFrag_worldSpace.z);

    if (zAbs >= xAbs && zAbs >= yAbs)
    {
        lightToFrag_worldSpace.x = -lightToFrag_worldSpace.x;
    }
    else
    {
        lightToFrag_worldSpace.z = -lightToFrag_worldSpace.z;
    }

    // PCF filter
    const vec3 sampleOffsetDirections[20] = vec3[]
    (
    vec3(1, 1, 1), vec3(1, -1, 1), vec3(-1, -1, 1), vec3(-1, 1, 1),
    vec3(1, 1,-1), vec3(1,-1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
    vec3(1, 1, 0), vec3(1,-1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0),
    vec3(1, 0, 1), vec3(-1, 0, 1), vec3(1, 0, -1), vec3(-1, 0, -1),
    vec3(0, 1, 1), vec3(0, -1, 1), vec3(0, -1, -1), vec3(0, 1, -1)
    );

    const int numSamples = 20;
    const float diskRadius = (1.0 + (length(lightToFrag_worldSpace) / lightData.maxAffectRange)) / 100.0;

    float shadow = 0.0;

    for (int i = 0; i < numSamples; ++i)
    {
        const float pcfFragDepth = texture(
        i_shadowSampler_cubeMap[lightData.shadowMapIndex],
        lightToFrag_worldSpace + sampleOffsetDirections[i] * diskRadius
        ).r;

        shadow += lightToFragDepth > pcfFragDepth ? 1.0 : 0.0;
    }

    return shadow / float(numSamples);
}

bool IsPointWithinLightCone(LightPayload light, vec3 point_worldSpace)
{
    const vec3 lightToPoint_worldSpaceUnit = normalize(point_worldSpace - light.worldPos);
    const float vectorAlignment = dot(light.directionUnit, lightToPoint_worldSpaceUnit);

    const float alignmentAngleDegrees = degrees(acos(vectorAlignment));

    return alignmentAngleDegrees <= light.coneFovDegrees / 2.0f;
}
