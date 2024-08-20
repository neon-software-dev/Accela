/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#version 460

//
// INPUTS
//
layout(location = 0) in vec3 i_fragTexCoord;            // The fragment's tex coord

// Set 2 - Texture bindings
layout(set = 2, binding = 0) uniform samplerCube i_skyboxSampler;

//
// OUTPUTS
//
layout(location = 0) out vec4 o_fragColor;
layout(location = 1) out uvec2 o_vertexObjectDetail;

void main()
{
    o_fragColor = texture(i_skyboxSampler, i_fragTexCoord);
    o_vertexObjectDetail.r = 0;
    o_vertexObjectDetail.g = 0;
}
