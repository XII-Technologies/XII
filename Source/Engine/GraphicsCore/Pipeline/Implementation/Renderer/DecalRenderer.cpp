#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Renderer/DecalRenderer.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDecalRenderer, 1, xiiRTTIDefaultAllocator<xiiDecalRenderer>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiDecalRenderer::xiiDecalRenderer() = default;

xiiDecalRenderer::~xiiDecalRenderer() = default;

void xiiDecalRenderer::GetSupportedRenderDataTypes(xiiDynamicArray<const xiiRTTI*>& out_types) const
{
  XII_IGNORE_UNUSED(out_types);
}

void xiiDecalRenderer::RenderBatch(const xiiRenderViewContext& renderContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const
{
  XII_IGNORE_UNUSED(renderContext);
  XII_IGNORE_UNUSED(pPass);
  XII_IGNORE_UNUSED(batch);
}
