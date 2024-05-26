#include "RenderState.h"

namespace Accela::Render
{

void RenderState::OnPipelineBound(const ProgramDefPtr& _programDef, const VulkanPipelinePtr& _pipeline)
{
    programDef = _programDef;
    pipeline = _pipeline;

    set0Invalidated = true;
    set1Invalidated = true;
    set2Invalidated = true;
    set3Invalidated = true;
}

void RenderState::OnVertexBufferBound(const BufferPtr& buffer)
{
    vertexBuffer = buffer;
}

void RenderState::OnIndexBufferBound(const BufferPtr& buffer)
{
    indexBuffer = buffer;
}

void RenderState::OnSet0Bound()
{
    set0Invalidated = false;
    set1Invalidated = true;
    set2Invalidated = true;
    set3Invalidated = true;
}

void RenderState::OnSet1Bound()
{
    set1Invalidated = false;
    set2Invalidated = true;
    set3Invalidated = true;
}

void RenderState::OnSet2Bound()
{
    set2Invalidated = false;
    set3Invalidated = true;
}

void RenderState::OnSet3Bound()
{
    set3Invalidated = false;
}

}
