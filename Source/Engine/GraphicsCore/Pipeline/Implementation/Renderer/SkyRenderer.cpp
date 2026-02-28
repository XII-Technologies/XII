#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Renderer/SkyRenderer.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSkyRenderer, 1, xiiRTTIDefaultAllocator<xiiSkyRenderer>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiSkyRenderer::xiiSkyRenderer() = default;

xiiSkyRenderer::~xiiSkyRenderer() = default;

void xiiSkyRenderer::GetSupportedRenderDataTypes(xiiDynamicArray<const xiiRTTI*>& out_types) const
{
  XII_IGNORE_UNUSED(out_types);
}

void xiiSkyRenderer::RenderBatch(const xiiRenderViewContext& renderContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const
{
  XII_IGNORE_UNUSED(renderContext);
  XII_IGNORE_UNUSED(pPass);
  XII_IGNORE_UNUSED(batch);
}
