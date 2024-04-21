/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
 
#version 460

//
// Definitions
//

//
// INPUTS
//

// Vertex Data
layout(location = 0) in vec3 i_vertexPosition_modelSpace;
layout(location = 1) in vec3 i_vertexNormal_modelSpace;
layout(location = 2) in vec2 i_vertexUv;
layout(location = 3) in vec3 i_vertexTangent_modelSpace;

//
// OUTPUTS
//
layout(location = 0) out vec2 o_fragTexCoord;           // The vertex's tex coord

void main()
{
    gl_Position = vec4(i_vertexPosition_modelSpace, 1.0);
    o_fragTexCoord = i_vertexUv;
}