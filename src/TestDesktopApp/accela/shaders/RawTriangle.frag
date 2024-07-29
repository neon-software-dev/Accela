/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#version 460

//
// INPUTS
//

//
// OUTPUTS
//
layout(location = 0) out vec4 o_fragColor;
layout(location = 1) out uvec2 o_vertexObjectDetail;

void main()
{
    o_fragColor = vec4(0, 1, 0, 1);

    // Fake object detail as raw triangles have no object associated with them
    o_vertexObjectDetail = uvec2(0, 0);
}
