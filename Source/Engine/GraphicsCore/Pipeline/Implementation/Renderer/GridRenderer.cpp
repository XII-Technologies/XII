#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Renderer/GridRenderer.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGridRenderer, 1, xiiRTTIDefaultAllocator<xiiGridRenderer>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiGridRenderer::xiiGridRenderer() = default;

xiiGridRenderer::~xiiGridRenderer() = default;

void xiiGridRenderer::GetSupportedRenderDataTypes(xiiDynamicArray<const xiiRTTI*>& out_types) const
{
  XII_IGNORE_UNUSED(out_types);
}

void xiiGridRenderer::RenderBatch(const xiiRenderViewContext& renderContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const
{
  XII_IGNORE_UNUSED(renderContext);
  XII_IGNORE_UNUSED(pPass);
  XII_IGNORE_UNUSED(batch);
}
