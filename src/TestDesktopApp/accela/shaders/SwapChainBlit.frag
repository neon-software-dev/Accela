/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#version 460

//
// INPUTS
//
layout(push_constant) uniform constants
{
    uint presentEyeIndex;
} PushConstants;

layout(location = 0) in vec2 i_fragTexCoord;            // The fragment's tex coord

layout(set = 0, binding = 0) uniform sampler2DArray i_renderSampler;
layout(set = 0, binding = 1) uniform sampler2DArray i_screenSampler;

//
// OUTPUTS
//
layout(location = 0) out vec4 o_fragColor;

void main()
{
    // Sample from the world render output
    const vec4 renderColor = texture(i_renderSampler, vec3(i_fragTexCoord, PushConstants.presentEyeIndex));

    // Sample from the screen/sprite output. Note that screen image only ever has 1 layer.
    const vec4 screenColor = texture(i_screenSampler, vec3(i_fragTexCoord, 0));

    // Combine sprite output on top of the render output using additive one-minus blending
    const vec4 finalColor = (screenColor * screenColor.a) + (renderColor * (1.0f - screenColor.a));

    o_fragColor = vec4(finalColor.r, finalColor.g, finalColor.b, 1.0f);
}