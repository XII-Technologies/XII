#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Passes/SkyRenderPass.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSkyRenderPass, 1, xiiRTTIDefaultAllocator<xiiSkyRenderPass>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiSkyRenderPass::xiiSkyRenderPass(const char* szName) :
  xiiForwardRenderPass(szName)
{
}

xiiSkyRenderPass::~xiiSkyRenderPass() = default;

void xiiSkyRenderPass::RenderObjects(const xiiRenderViewContext& renderViewContext)
{
  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::Sky);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Passes_SkyRenderPass);
