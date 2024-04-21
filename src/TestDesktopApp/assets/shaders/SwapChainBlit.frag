/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#version 460

//
// INPUTS
//
layout(location = 0) in vec2 i_fragTexCoord;            // The fragment's tex coord

layout(set = 0, binding = 0) uniform sampler2DArray i_offscreenSampler;

//
// OUTPUTS
//
layout(location = 0) out vec4 o_fragColor;

void main()
{
    // TODO: Make it configurable which layer we're sampling from, so in VR we can configure whether
    // to show the left or right eye on screen. Defaulting to left for now.
    const vec4 sampledColor = texture(i_offscreenSampler, vec3(i_fragTexCoord, 0));

    o_fragColor = vec4(sampledColor.r, sampledColor.g, sampledColor.b, 1.0f);
}