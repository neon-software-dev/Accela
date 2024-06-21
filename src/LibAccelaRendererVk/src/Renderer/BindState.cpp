/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "BindState.h"

namespace Accela::Render
{

void BindState::OnPipelineBound(const ProgramDefPtr& _programDef, const VulkanPipelinePtr& _pipeline)
{
    programDef = _programDef;
    pipeline = _pipeline;

    set0Invalidated = true;
    set1Invalidated = true;
    set2Invalidated = true;
    set3Invalidated = true;
}

void BindState::OnVertexBufferBound(const BufferPtr& buffer)
{
    vertexBuffer = buffer;
}

void BindState::OnIndexBufferBound(const BufferPtr& buffer)
{
    indexBuffer = buffer;
}

void BindState::OnSet0Bound()
{
    set0Invalidated = false;
    set1Invalidated = true;
    set2Invalidated = true;
    set3Invalidated = true;
}

void BindState::OnSet1Bound()
{
    set1Invalidated = false;
    set2Invalidated = true;
    set3Invalidated = true;
}

void BindState::OnSet2Bound()
{
    set2Invalidated = false;
    set3Invalidated = true;
}

void BindState::OnSet3Bound()
{
    set3Invalidated = false;
}

}
