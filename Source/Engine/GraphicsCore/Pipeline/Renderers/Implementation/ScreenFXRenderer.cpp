#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Pipeline/Renderers/ScreenFXRenderer.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiScreenFXRenderer, 1, xiiRTTIDefaultAllocator<xiiScreenFXRenderer>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiScreenFXRenderer::xiiScreenFXRenderer() = default;

xiiScreenFXRenderer::~xiiScreenFXRenderer() = default;

void xiiScreenFXRenderer::GetSupportedRenderDataTypes(xiiDynamicArray<const xiiRTTI*>& out_types) const
{
  XII_IGNORE_UNUSED(out_types);
}

void xiiScreenFXRenderer::RenderBatch(const xiiRenderViewContext& renderContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const
{
  XII_IGNORE_UNUSED(renderContext);
  XII_IGNORE_UNUSED(pPass);
  XII_IGNORE_UNUSED(batch);
}
