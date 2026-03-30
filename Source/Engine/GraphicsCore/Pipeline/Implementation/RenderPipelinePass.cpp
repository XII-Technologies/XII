#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/RenderPipelinePass.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelinePass, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiRenderPipelinePass::xiiRenderPipelinePass(xiiStringView sName, bool bActive) :
  m_sName(sName), m_bActive(bActive)
{
}

xiiRenderPipelinePass::~xiiRenderPipelinePass() = default;
