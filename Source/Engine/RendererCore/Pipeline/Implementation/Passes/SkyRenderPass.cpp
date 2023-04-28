#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/Passes/SkyRenderPass.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSkyRenderPass, 1, xiiRTTIDefaultAllocator<xiiSkyRenderPass>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiSkyRenderPass::xiiSkyRenderPass(const char* szName) :
  xiiForwardRenderPass(szName)
{
}

xiiSkyRenderPass::~xiiSkyRenderPass() {}

void xiiSkyRenderPass::RenderObjects(const xiiRenderViewContext& renderViewContext)
{
  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::Sky);
}


XII_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SkyRenderPass);
