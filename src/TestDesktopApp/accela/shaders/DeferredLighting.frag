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

CalculatedLight CalculateFragmentLighting(MaterialPayload fragmentMaterial);
float GetFragShadowLevel(LightPayload lightData, vec3 fragPosition_worldSpace);
float GetLightFragDepth(LightPayload lightData, vec3 fragPosition_worldSpace);
float GetFragShadowLevel_Single(LightPayload lightData, vec3 fragPosition_worldSpace, float lightToFragDepth);
float GetFragShadowLevel_Cube(LightPayload lightData, vec3 fragPosition_worldSpace, float lightToFragDepth);
bool IsPointWithinLightCone(LightPayload light, vec3 point_worldSpace);

float MapRange(float val, float min1, float max1, float min2, float max2)
{
    return min2 + (val / (max1 - min1)) * (max2 - min2);
}

//
// INPUTS
layout(input_attachment_index = 0, set = 1, binding = 0) uniform subpassInput i_vertexPosition_worldSpace;
layout(input_attachment_index = 1, set = 1, binding = 1) uniform subpassInput i_vertexNormal_viewSpace;
layout(input_attachment_index = 2, set = 1, binding = 2) uniform usubpassInput i_vertexMaterial;
layout(input_attachment_index = 3, set = 1, binding = 3) uniform subpassInput i_vertexAmbientColor;
layout(input_attachment_index = 4, set = 1, binding = 4) uniform subpassInput i_vertexDiffuseColor;
layout(input_attachment_index = 5, set = 1, binding = 5) uniform subpassInput i_vertexSpecularColor;

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

// Set 2 - Material Data
layout(set = 2, binding = 0) readonly buffer MaterialPayloadBuffer
{
    MaterialPayload data[];
} i_materialData;

layout(push_constant) uniform constants
{
    bool hdr;
} PushConstants;

//
// OUTPUTS
//
layout(location = 0) out vec4 o_fragColor;

void main()
{
    const uint fragmentMaterialIndex = subpassLoad(i_vertexMaterial).r;
    const MaterialPayload fragmentMaterial = i_materialData.data[fragmentMaterialIndex];

    //
    // Fetch the fragment's material colors as output by the gpass
    //
    FragmentColors fragmentColors;
    fragmentColors.ambientColor = subpassLoad(i_vertexAmbientColor).rgba;
    fragmentColors.diffuseColor = subpassLoad(i_vertexDiffuseColor).rgba;
    fragmentColors.specularColor = subpassLoad(i_vertexSpecularColor).rgba;

    //
    // Calculate the light hitting the fragment
    //
    const CalculatedLight calculatedLight = CalculateFragmentLighting(fragmentMaterial);

    //
    // Combine the lighting with the fragment's color
    //
    const vec4 ambientColor = fragmentColors.ambientColor * vec4(calculatedLight.ambientLight, 1.0f);
    const vec4 diffuseColor = fragmentColors.diffuseColor * vec4(calculatedLight.diffuseLight, 1.0f);
    const vec4 specularColor = fragmentColors.specularColor * vec4(calculatedLight.specularLight, 1.0f);

    vec4 surfaceColor = ambientColor + diffuseColor + specularColor;

    //
    // If not doing HDR, clamp the brighest color between pure dark and pure bright
    //
    if (!PushConstants.hdr)
    {
        surfaceColor = clamp(surfaceColor, vec4(0, 0, 0, 0), vec4(1, 1, 1, 1));
    }

    o_fragColor = surfaceColor;
}

CalculatedLight CalculateFragmentLighting(MaterialPayload fragmentMaterial)
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
    const vec3 fragPosition_worldSpace = subpassLoad(i_vertexPosition_worldSpace).rgb;
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
        const vec3 vertexNormal_viewSpace = subpassLoad(i_vertexNormal_viewSpace).rgb;

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

    // Convert to cube-map left-handed coordinate system with swapped z-axis
    lightToFrag_worldSpace.z *= -1.0f;

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
