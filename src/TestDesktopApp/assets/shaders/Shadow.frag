/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#version 460

//
// Inputs
//
layout(push_constant) uniform constants
{
    float lightMaxAffectRange;
} PushConstants;

layout(location = 0) in vec3 i_vertexPosition_shadowViewSpace;

void main()
{
    // Distance from the light to the vertex
    float lightDistance = length(i_vertexPosition_shadowViewSpace);

    // Map world distance to linear [0,1] depth range by dividing frag distance by max light range
    // TODO Quality: Use non-linear depth calculation. Update GetLightFragDepth in DeferredLighting.frag appropriately as well.
    lightDistance = lightDistance / PushConstants.lightMaxAffectRange;

    //
    // Output
    //
    gl_FragDepth = lightDistance;
}
